// Pulsar Audio Engine
// Copyright 2019 Tyler Riddle <kg7oem@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

#include <chrono>

#include <pulsar/audio.util.h>
#include <pulsar/debug.h>
#include <pulsar/jackaudio.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

#define JACK_LOG_NAME "jackaudio"

namespace pulsar {

using namespace std::chrono_literals;

namespace jackaudio {

static void log_jack_info(const char * jack_message_in)
{
    LOGJAM_LOG_VARGS(JACK_LOG_NAME, logjam::loglevel::info, jack_message_in);
}

static void log_jack_error(const char * jack_message_in)
{
    LOGJAM_LOG_VARGS(JACK_LOG_NAME, logjam::loglevel::error, jack_message_in);
}

void init()
{
    jack_set_error_function(log_jack_error);
    jack_set_info_function(log_jack_info);

    library::register_node_factory("pulsar::jackaudio::node", make_node);
    library::register_daemon_factory("pulsar::jackaudio::connections", make_connections);
}

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<jackaudio::node>(name_in);
}

std::shared_ptr<connections> make_connections(const string_type& name_in)
{
    return std::make_shared<connections>(name_in);
}

} // namespace jackaudio

jackaudio::node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).set("pulsar::jackaudio::node");
    add_property("config:client_name", property::value_type::string).set(name_in);
    add_property("config:sample_rate", property::value_type::size);
    add_property("config:watchdog_timeout_ms", property::value_type::size);
}

jackaudio::node::~node()
{
    for (auto&& port : jack_ports) {
        jack_port_unregister(jack_client, port.second);
    }

    jack_ports.empty();
}

void jackaudio::node::open(const string_type& jack_name_in)
{
    log_trace("opening jackaudio client");

    jack_client = jack_client_open(jack_name_in.c_str(), jack_options, 0);

    if (jack_client == nullptr) {
        system_fault("could not open connection to jack server");
    }

    auto sample_rate = jack_get_sample_rate(jack_client);
    auto client_name = jack_get_client_name(jack_client);

    if (sample_rate != domain->sample_rate) {
        system_fault("jack sample rate did not match domain sample rate");
    }

    get_property("config:client_name").set(client_name);
    get_property("config:sample_rate").set(sample_rate);
}

jackaudio::port_type * jackaudio::node::add_port(const string_type& port_name_in, const char * port_type_in, const flags_type flags_in, const size_type buffer_size_in)
{
    if (jack_ports.count(port_name_in) != 0) {
        system_fault("attempt to register duplicate jackaudio port name: " + port_name_in);
    }

    auto new_port = jack_port_register(jack_client, port_name_in.c_str(), port_type_in, flags_in, buffer_size_in);

    if (new_port == nullptr) {
        system_fault("could not create a jackaudio port named " + port_name_in);
    }

    jack_ports[port_name_in] = new_port;

    return new_port;
}

pulsar::sample_type * jackaudio::node::get_port_buffer(const string_type& name_in)
{
    if (jack_ports.count(name_in) == 0) {
        system_fault("could not find a jackaudio port named " + name_in);
    }

    auto port = jack_ports[name_in];
    auto buffer = jack_port_get_buffer(port, domain->buffer_size);
    return static_cast<pulsar::sample_type *>(buffer);
}

static int wrap_int_nframes_cb(jackaudio::jack_nframes_t arg_in, void * cb_pointer)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<int(jackaudio::jack_nframes_t)> *>(cb_pointer);
    auto cb = *p;
    cb(arg_in);
    return 0;
}

static void wrap_void_status_string_cb(jackaudio::jack_status_t status_in, const char * message_in, void * cb_pointer)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<void(jackaudio::jack_status_t, const char * message_in)> *>(cb_pointer);
    auto cb = *p;
    cb(status_in, message_in);
    return;
}

static void wrap_uint32_int_cb(const uint32_t uint32_in, const int register_in, void * arg)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<void(const uint32_t, const int)> *>(arg);
    auto cb = *p;
    cb(uint32_in, register_in);
}

void jackaudio::node::activate()
{
    log_trace("activating the jackaudio node: ", name);

    assert(jack_client == nullptr);

    auto& client_name = get_property("config:client_name").get_string();
    open(client_name);

    for(auto&& name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        add_port(output->name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    }

    for(auto&& name : audio.get_input_names()) {
        auto input = audio.get_input(name);
        add_port(input->name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    }

    jack_on_info_shutdown(
        jack_client,
        wrap_void_status_string_cb,
        static_cast<void *>(new std::function<void(jackaudio::jack_status_t, const char *)>([this](jackaudio::jack_status_t status_in, const char * message_in) -> void {
            this->handle_jack_shutdown(status_in, message_in);
    })));

    if (jack_set_process_callback(
        jack_client,
        wrap_int_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            this->handle_jack_process(nframes_in);
    })))) {
        system_fault("could not set jackaudio process callback");
    }

    pulsar::node::base::activate();
}

// FIXME right now a node with no inputs will never run.
// This needs to be fixed at the node level so JACK can
// work if it supplies audio only.
void jackaudio::node::handle_jack_process(jack_nframes_t nframes_in)
{
    log_debug("********** jackaudio process callback invoked");

    if (watchdog != nullptr) watchdog->start();

    auto lock = debug_get_lock(node_mutex);
    auto done_lock = debug_get_lock(done_mutex);

    if (done_flag) {
        system_fault("jackaudio handle_jack_process() went reentrant");
    }

    done_lock.unlock();

    if (nframes_in != domain->buffer_size) {
        system_fault("jackaudio process request and buffer size were not the same");
    }

    for(auto&& name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        auto jack_buffer = get_port_buffer(name);
        auto buffer = audio::buffer::make();

        buffer->init(nframes_in, jack_buffer);
        output->set_buffer(buffer);
    }

    lock.unlock();

    log_trace("waiting for jackaudio node to become done");
    debug_relock(done_lock);
    done_cond.wait(done_lock, [this]{ return done_flag; });

    done_flag = false;
    log_trace("going to reset watchdog for node", name);
    if (watchdog != nullptr) watchdog->stop();
    log_debug("********** giving control back to jackaudio");
}

void jackaudio::node::handle_jack_shutdown(jack_status_t, const char * message_in)
{
    system_fault("jackaudio server shut down: ", message_in);
}

void jackaudio::node::run()
{
    log_trace("jackaudio node is now running");

    for(auto&& name : audio.get_input_names()) {
        auto buffer_size = domain->buffer_size;
        auto input = audio.get_input(name);
        auto jack_buffer = get_port_buffer(name);
        auto channel_buffer = input->get_buffer();
        audio::util::pcm_set(jack_buffer, channel_buffer->get_pointer(), buffer_size);
    }

    pulsar::node::base::run();
}

// notifications happened from inside the jackaudio callback
void jackaudio::node::notify()
{ }

void jackaudio::node::start()
{
    log_trace("starting jackaudio node: ", name);

    if (jack_client == nullptr) system_fault("jackaudio client pointer was null");

    auto watchdog_timeout = get_property("config:watchdog_timeout_ms").get_size();

    if (watchdog_timeout) {
        auto message = util::to_string("jackaudio node ", name, " was not ready fast enough");
        auto duration = std::chrono::milliseconds(watchdog_timeout);
        watchdog = async::watchdog::make(duration, message);
    }

    if (jack_activate(jack_client)) {
        system_fault("could not activate jack client");
    }
}

void jackaudio::node::execute()
{
    log_trace("invoking parent execute() method first for jackaudio node");
    node::base::execute();

    log_trace("waking up jackaudio thread");
    auto done_lock = debug_get_lock(done_mutex);
    done_flag = true;
    done_cond.notify_all();
}

void jackaudio::node::stop()
{
    log_trace("stopping jackaudio");

    if (jack_deactivate(jack_client) != 0) {
        system_fault("could not deactivate jack client");
    }

    node::base::stop();
}

// start following new convention
namespace jackaudio {

connections::connections(const string_type& name_in)
: daemon::base(name_in)
{ }

connections::~connections()
{
    if (jack_client != nullptr) {
        stop();
    }
}

void connections::init(const YAML::Node& yaml_in)
{
    log_debug("initializing jackaudio connection daemon");

    auto connect_node = yaml_in["connect"];
    for(size_type i = 0; i < connect_node.size(); i++) {
        auto connection_node = connect_node[i];
        auto from = connection_node[0].as<string_type>();
        auto to = connection_node[1].as<string_type>();

        log_info("jackaudio: watching connection ", from, " -> ", to);
        connection_list.emplace_back(from, to);
    }
}

void connections::register_callbacks()
{
    if(jack_set_port_registration_callback(
        jack_client,
        wrap_uint32_int_cb,
        static_cast<void *>(new std::function<void(const uint32_t port_id_in, const int register_in)>([this](const uint32_t UNUSED port_id_in, const int register_in) -> void {
            auto jack_port = jack_port_by_id(jack_client, port_id_in);
            auto port_name = jack_port_name(jack_port);

            if (register_in) {
                log_debug("jackaudio: new port registration: ", port_name);
                // jackaudio does not like calling it from a notification
                // thread: "Cannot callback the server in notification thread!"
                async::submit_job(&connections::check_port_connections, this, port_name);
            } else {
                log_debug("jackaudio: port is being deregistered: ", port_name);
            }
    }))))
    {
        system_fault("could not register jack port registration callback");
    }
}

void connections::start()
{
    log_info("starting jackaudio connection daemon");

    {
        auto lock = debug_get_lock(jack_mutex);

        assert(jack_client == nullptr);

        jack_client = jack_client_open("pulsar_connections", jack_options, 0);

        if (jack_client == nullptr) {
            system_fault("could not open connection to jack server");
        }

        register_callbacks();

        if (jack_activate(jack_client)) {
            system_fault("could not activate jack client");
        }
    }

    // do the initial connecting
    for (auto&& i : connection_list) {
        check_port_connections(i.first);
    }
}

void connections::stop()
{
    auto lock = debug_get_lock(jack_mutex);

    assert(jack_client != nullptr);

    if (jack_deactivate(jack_client)) {
        system_fault("could not deactivate jackaudio client");
    }

    if (jack_client_close(jack_client)) {
        system_fault("could not close jackaudio client");
    }

    jack_client = nullptr;
}

void connections::check_port_connections(const string_type&)
{
    auto lock = debug_get_lock(jack_mutex);

    // FIXME Attempt to connect everything known about and ignore
    // any stuff like ports not existing or errors that occur.
    // This isn't really good enough but it's good enough.
    for(auto&& i : connection_list) {
        auto output = i.first;
        auto input = i.second;
        log_debug("connecting jack ports: ", output, " -> ", input);
        jack_connect(jack_client, output.c_str(), input.c_str());
    }
}

} // namespace jackaudio

} // namespace pulsar

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
#include <pulsar/jackaudio.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

#define WATCHDOG_TIMEOUT 1500ms

namespace pulsar {

using namespace std::chrono_literals;

namespace jackaudio {

void init()
{
    library::register_node_factory("pulsar::jackaudio::node", make_node);
}

pulsar::node::base::node * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<jackaudio::node>(name_in);
}

} // namespace jackaudio


jackaudio::node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base::node(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).set("pulsar::jackaudio::node");
    add_property("config:client_name", property::value_type::string).set(name_in);
    add_property("config:sample_rate", property::value_type::size);
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

static int wrap_nframes_cb(jackaudio::jack_nframes_t nframes_in, void * arg_in)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<int(jackaudio::jack_nframes_t)> *>(arg_in);
    auto cb = *p;
    cb(nframes_in);
    return 0;
}

void jackaudio::node::activate()
{
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

    if (jack_set_process_callback(
        jack_client,
        wrap_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            this->handle_jack_process(nframes_in);
    })))) {
        system_fault("could not set jackaudio process callback");
    }

    start();

    pulsar::node::base::node::activate();
}

// FIXME right now a node with no inputs will never run.
// This needs to be fixed at the node level so JACK can
// work if it supplies audio only.
void jackaudio::node::handle_jack_process(jack_nframes_t nframes_in)
{
    log_trace("********** jackaudio process callback invoked");

    auto lock = lock_type(node_mutex);
    auto done_lock = lock_type(done_mutex);

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
        auto buffer = std::make_shared<audio::buffer>();

        buffer->init(nframes_in, jack_buffer);
        output->set_buffer(buffer);
    }

    lock.unlock();

    log_trace("waiting for jackaudio node to become done");
    done_lock.lock();
    done_cond.wait(done_lock, [this]{ return done_flag; });

    done_flag = false;
    watchdog->reset();
    log_trace("********** giving control back to jackaudio");
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

    auto done_lock = lock_type(done_mutex);
    done_flag = true;
    done_cond.notify_all();

    pulsar::node::base::node::run();
}

// notifications happened from inside the jackaudio callback
void jackaudio::node::notify()
{ }

void jackaudio::node::start()
{
    watchdog = async::watchdog::make(WATCHDOG_TIMEOUT);

    if (jack_activate(jack_client)) {
        system_fault("could not activate jack client");
    }

    watchdog->start();
}

} // namespace pulsar

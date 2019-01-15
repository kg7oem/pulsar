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

#include <iostream>

#include "audio.util.h"
#include "jackaudio.h"

namespace pulsar {

jackaudio::node::node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: node::base(name_in, domain_in)
{ }

jackaudio::node::~node()
{
    for (auto port : jack_ports) {
        jack_port_unregister(jack_client, port.second);
    }

    jack_ports.empty();
}

node::base::lock_type jackaudio::node::make_done_lock()
{
    return node::lock_type(done_mutex);
}

void jackaudio::node::reset()
{

    node::base::reset();
}

void jackaudio::node::open()
{
    open(name);
}

void jackaudio::node::open(const std::string& jack_name_in)
{
    jack_client = jack_client_open(jack_name_in.c_str(), jack_options, 0);

    if (jack_client == nullptr) {
        throw std::runtime_error("could not open connection to jack server");
    }

    if (jack_get_sample_rate(jack_client) != domain->sample_rate) {
        throw std::runtime_error("jack sample rate did not match domain sample rate");
    }
}

jackaudio::port_type * jackaudio::node::add_port(const std::string& port_name_in, const char * port_type_in, const flags_type flags_in, const size_type buffer_size_in)
{
    if (jack_ports.count(port_name_in) != 0) {
        throw std::runtime_error("attempt to register duplicate jackaudio port name: " + port_name_in);
    }

    auto new_port = jack_port_register(jack_client, port_name_in.c_str(), port_type_in, flags_in, buffer_size_in);

    if (new_port == nullptr) {
        throw std::runtime_error("could not create a jackaudio port named " + port_name_in);
    }

    jack_ports[port_name_in] = new_port;

    return new_port;
}

pulsar::sample_type * jackaudio::node::get_port_buffer(const std::string& name_in)
{
    if (jack_ports.count(name_in) == 0) {
        throw std::runtime_error("could not find a jackaudio port named " + name_in);
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

void jackaudio::node::handle_activate()
{
    if (jack_client == nullptr) {
        open();
    }

    for(auto name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        add_port(output->name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    }

    for(auto name : audio.get_input_names()) {
        auto input = audio.get_input(name);
        add_port(input->name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    }

    if (jack_set_process_callback(
        jack_client,
        wrap_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            this->handle_jack_process(nframes_in);
    })))) {
        throw std::runtime_error("could not set jackaudio process callback");
    }

    start();
}

void jackaudio::node::handle_jack_process(jack_nframes_t nframes_in)
{
    std::cout << std::endl << "jackaudio process callback invoked" << std::endl;

    auto lock = make_lock();
    auto done_lock = make_done_lock();

    if (done_flag) {
        throw std::runtime_error("jackaudio handle_jack_process() went reentrant");
    }

    done_lock.unlock();

    if (nframes_in != domain->buffer_size) {
        throw std::runtime_error("jackaudio process request and buffer size were not the same");
    }

    for(auto name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        auto jack_buffer = get_port_buffer(name);
        output->get_buffer()->set(jack_buffer, nframes_in);
        output->notify();
    }

    // if the node is ready now it won't get a chance to be put into the ready
    // node list later
    if (is_ready()) {
        handle_ready();
    }

    lock.unlock();

    std::cout << "waiting for jackaudio node to become done" << std::endl;
    done_lock.lock();
    done_cond.wait(done_lock, [this]{ return done_flag; });

    done_flag = false;
    std::cout << "giving control back to jackaudio" << std::endl;
}

void jackaudio::node::handle_run()
{
    std::cout << "jackaudio node is now running" << std::endl;

    for(auto name : audio.get_input_names()) {
        auto buffer_size = domain->buffer_size;
        auto input = audio.get_input(name);
        auto jack_buffer = get_port_buffer(name);
        auto channel_buffer = input->get_pointer();
        audio::util::pcm_set(jack_buffer, channel_buffer, buffer_size);
    }

    std::cout << "notifying jackaudio done_flag condition variable" << std::endl;
    auto done_lock = make_done_lock();
    done_flag = true;
    done_cond.notify_all();
}

void jackaudio::node::start()
{
    if (jack_activate(jack_client)) {
        throw std::runtime_error("could not activate jack client");
    }
}

} // namespace pulsar

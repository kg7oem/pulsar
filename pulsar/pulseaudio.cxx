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

#include <pulsar/library.h>
#include <pulsar/logging.h>
#include <pulsar/pulseaudio.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace pulseaudio {

static pa_mainloop * pulse_main_loop = nullptr;
static pa_mainloop_api * pulse_api = nullptr;
static thread_type * pulse_thread = nullptr;

static void pulse_loop()
{
    assert(pulse_main_loop != nullptr);

    log_debug("pulseaudio thread is giving control to pulseaudio main loop");
    auto result = pa_mainloop_run(pulse_main_loop, NULL);
    log_debug("pulseaudio thread got control back from pulseaudio main loop");

    if (result < 0) {
        system_fault("pulseaudio pa_mainloop_run() had an error response");
    }
}

pulsar::node::base * make_client_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<pulseaudio::client_node>(name_in);
}

void init()
{
    log_debug("pulseaudio system is initializing");

    assert(pulse_thread == nullptr);
    assert(pulse_api == nullptr);
    assert(pulse_main_loop == nullptr);

    pulse_main_loop = pa_mainloop_new();
    assert(pulse_main_loop != nullptr);

    pulse_api = pa_mainloop_get_api(pulse_main_loop);
    assert(pulse_api != nullptr);

    pulse_thread = new thread_type(pulse_loop);

    library::register_node_factory("pulsar::pulseaudio::client", make_client_node);
}

pa_context * make_context(const string_type& name_in)
{
    assert(pulse_api != nullptr);

    auto context = pa_context_new(pulse_api, name_in.c_str());
    assert(context != nullptr);

    return context;
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).set("pulsar::jackaudio::node");

    add_property("config:context", property::value_type::string).set(name_in);
    add_property("config:sample_rate", property::value_type::size);
}

client_node::client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: node(name_in, domain_in)
{ }

} // namespace pulseaudio

} // namespace pulsar

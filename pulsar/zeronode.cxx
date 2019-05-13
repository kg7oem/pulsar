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

#include <pulsar/audio.util.h>
#include <pulsar/async.h>
#include <pulsar/logging.h>
#include <pulsar/zeronode.h>

namespace pulsar {

namespace zeronode {

void init()
{
    log_debug("Initializing zero node");

    library::register_node_factory("pulsar::zeronode::node", make_node);
}

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<zeronode::node>(name_in);
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::io(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).value->set("pulsar::portaudio::node");
    add_property("config:hz", property::value_type::integer);
    add_property("config:max_cycles", property::value_type::integer);
    add_property("state:cycle_num", property::value_type::integer);
}

void node::activate()
{
    assert(timer == nullptr);

    auto hz_property = get_property("config:hz");

    if (hz_property.value->get_integer() == 0) {
        hz_property.value->set(domain->sample_rate / domain->buffer_size);
    }

    auto duration = duration_type(1000 / hz_property.value->get_integer());
    timer = async::timer::make(duration, duration);
    timer->watch([this] (async::base_timer&) { this->handle_timer(); });

    pulsar::node::io::activate();
}

void node::start()
{
    timer->start();

    pulsar::node::io::start();
}

void node::execute()
{ }

void node::handle_timer()
{
    log_trace("***************** zero node timer callback was invoked");

    {
        auto busy_lock = pulsar_get_lock(busy_mutex);
        if (busy_flag) {
            system_fault("handle_timer() went reentrant for node ", name);
        }

        busy_flag = true;
    }

    auto zero_buffer = domain->get_zero_buffer();
    auto& cycle_num = get_property("state:cycle_num").value->get_integer();
    auto& max_cycles = get_property("config:max_cycles").value->get_integer();


    cycle_num++;

    for(auto&& name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        output->set_buffer(zero_buffer);
    }

    if (max_cycles != 0 && cycle_num > max_cycles) {
        log_debug("max_cycles met or exceeded, shutting down pulsar with async job");
        async::submit_job([] { pulsar::system::shutdown(); });
    }

    log_trace("***************** zero node timer callback is done");
}

void node::input_ready()
{
    log_trace("***************** zero node input ready callback was invoked");

    {
        auto busy_lock = pulsar_get_lock(busy_mutex);

        if (! busy_flag) {
            system_fault("busy_flag was not true inside input_ready() for node ", name);
        }
    }

    pulsar::node::io::input_ready();
    reset_cycle();

    {
        auto busy_lock = pulsar_get_lock(busy_mutex);
        busy_flag = false;
    }

    log_trace("***************** zero node input ready callback is done");
}

void node::stop()
{
    // timer->stop();

    pulsar::node::io::stop();
}

} // namespace zeronode

} // namespace pulsar

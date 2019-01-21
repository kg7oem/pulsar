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

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include <pulsar/library.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

namespace pulsar {

namespace node {

void init()
{
    library::register_node_factory("pulsar::node::chain", make_chain_node);
}

base::node * make_chain_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
{
    return domain_in->make_node<chain>(name_in);
}

base::node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in, const bool is_forwarder_in)
: domain(domain_in), name(name_in), is_forwarder(is_forwarder_in), audio(this)
{
    assert(domain != nullptr);

    add_property("node:name", property::value_type::string).set(name);
    add_property("node:domain", pulsar::property::value_type::string).set(domain->name);
}

base::node::~node()
{ }

std::shared_ptr<domain> base::node::get_domain()
{
    return domain;
}

property::generic& base::node::get_property(const string_type& name_in)
{
    auto result = properties.find(name_in);

    if (result == properties.end()) {
        system_fault("no property existed with name: ", name_in);
    }

    return *result->second;
}

string_type fully_qualify_property_name(const string_type& name_in)
{
    if (name_in.find(":") == string_type::npos) {
        return string_type("config:") + name_in;
    }

    return name_in;
}

string_type base::node::peek(const string_type& name_in)
{
    auto lock = lock_type(node_mutex);
    auto name = fully_qualify_property_name(name_in);
    return get_property(name).get();
}

const std::map<string_type, property::generic *>& base::node::get_properties()
{
    return properties;
}

property::generic& base::node::add_property(const string_type& name_in, const property::value_type& type_in)
{
    // FIXME why doesn't emplace work?
    auto new_property = new property::generic(name_in, type_in);
    properties[new_property->name] = new_property;
    return *new_property;
}

property::generic& base::node::add_property(const string_type& name_in, property::generic * property_in)
{
    properties[name_in] = property_in;
    return *property_in;
}

void base::node::activate()
{
    audio.activate();
    reset_cycle();
}

void base::node::init_cycle()
{
    log_trace("initializing cycle for node ", name);
    audio.init_cycle();
}

void base::node::will_run()
{
    init_cycle();
    domain->add_ready_node(this);
}

void base::node::run()
{ }

void base::node::did_run()
{ }

void base::node::notify()
{
    audio.notify();
}

void base::node::reset_cycle()
{
    audio.reset_cycle();
}

void base::node::deactivate()
{
    system_fault("cant deactivate yet");
}

void base::node::init()
{ }

void base::node::execute()
{
    auto lock = lock_type(node_mutex);

    run();
    did_run();
    notify();
    reset_cycle();
}

bool base::node::is_ready()
{
    return audio.is_ready();
}

forwarder::forwarder(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base::node(name_in, domain_in, true)
{ }

void forwarder::will_run()
{
    auto lock = lock_type(node_mutex);

    // a forwarder node does not use any CPU since all inputs and outputs
    // are forwarded but a full cycle still needs to happen so the
    // ready node queue can be skipped
    init_cycle();
    reset_cycle();
}

void forwarder::execute()
{
    system_fault("forwarder nodes should never try to execute");
}

// notifications happen via forwarding
void forwarder::notify()
{
    system_fault("forwarder nodes should never try to notify");
}

chain::chain(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: forwarder(name_in, domain_in)
{ }

} // namespace node

} // namespace pulsar

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

#include <pulsar/debug.h>
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

base * make_chain_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
{
    return domain_in->make_node<chain>(name_in);
}

static std::string make_dbus_path(const string_type& domain_name_in, const string_type& node_name_in)
{
    string_type buf;
    buf += "/Domain/" + domain_name_in;
    buf += "/Node/" + node_name_in;
    return buf;
}

dbus_node::dbus_node(base& parent_in, const string_type& domain_name_in, const string_type& node_name_in)
:
    DBus::ObjectAdaptor(dbus::get_connection(), make_dbus_path(domain_name_in, node_name_in)),
    parent(parent_in)
{ }

std::vector<string_type> dbus_node::property_names()
{
    std::vector<string_type> retval;

    for (auto&& i : parent.properties) {
        retval.push_back(i.first);
    }

    return retval;
}

std::map<string_type, string_type> dbus_node::properties()
{
    std::map<string_type, string_type> retval;

    for(auto&& i : parent.properties) {
        retval[i.first] = i.second->get();
    }

    return retval;
}

string_type dbus_node::peek(const string_type& name_in)
{
    return parent.peek(name_in);
}

base::base(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in, const bool is_forwarder_in)
: dbus(*this, domain_in->name, name_in), domain(domain_in), name(name_in), is_forwarder(is_forwarder_in), audio(this)
{
    assert(domain != nullptr);

    add_property("node:name", property::value_type::string).set(name);
    add_property("node:domain", pulsar::property::value_type::string).set(domain->name);
}

base::~base()
{ }

std::shared_ptr<domain> base::get_domain()
{
    return domain;
}

property::generic& base::get_property(const string_type& name_in)
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

string_type base::peek(const string_type& name_in)
{
    auto lock = debug_get_lock(node_mutex);
    auto name = fully_qualify_property_name(name_in);
    return get_property(name).get();
}

const std::map<string_type, property::generic *>& base::get_properties()
{
    return properties;
}

property::generic& base::add_property(const string_type& name_in, const property::value_type& type_in)
{
    // FIXME why doesn't emplace work?
    auto new_property = new property::generic(this, name_in, type_in);
    properties[new_property->name] = new_property;
    return *new_property;
}

property::generic& base::add_property(const string_type& name_in, property::generic * property_in)
{
    properties[name_in] = property_in;
    return *property_in;
}

void base::activate()
{
    log_debug("activating node ", name);

    audio.activate();
    reset_cycle();
}

void base::start()
{ }

void base::init_cycle()
{
    log_trace("initializing cycle for node ", name);
    audio.init_cycle();
}

void base::will_run()
{
    init_cycle();
    domain->add_ready_node(this);
}

void base::run()
{ }

void base::did_run()
{ }

void base::notify()
{
    audio.notify();
}

void base::reset_cycle()
{
    audio.reset_cycle();
}

void base::deactivate()
{
    system_fault("cant deactivate yet");
}

void base::init()
{ }

void base::execute()
{
    log_debug("--------> node ", name, " started executing");
    auto lock = debug_get_lock(node_mutex);

    run();
    did_run();

    // FIXME RACE There is still a race condition - after resetting new
    // buffers can start coming in again. This needs to get all the things
    // that will need to be notified into a list before the reset happens
    // then reset then notify using that copy of the data.
    reset_cycle();
    notify();

    log_debug("<-------- node ", name, " finished executing");
}

bool base::is_ready()
{
    return audio.is_ready();
}

forwarder::forwarder(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base(name_in, domain_in, true)
{ }

void forwarder::will_run()
{
    log_trace("forwarder node is short-circuting execute(): ", name);

    auto lock = debug_get_lock(node_mutex);

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

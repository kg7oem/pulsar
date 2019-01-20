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

#include "library.h"
#include "logging.h"
#include "node.h"
#include "system.h"

namespace pulsar {

namespace node {

void init()
{
    library::register_node_factory("pulsar::node::chain", make_chain_node);
}

base::node * make_chain_node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
{
    return domain_in->make_node<chain>(name_in);
}

base::node::node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in), name(name_in), audio(this)
{
    assert(domain != nullptr);

    add_property("node:name", property::value_type::string).set(name);
    add_property("node:domain", pulsar::property::value_type::string).set(domain->name);
}

base::node::~node()
{ }

base::node::lock_type base::node::make_lock()
{
    return lock_type(mutex);
}

std::shared_ptr<domain> base::node::get_domain()
{
    return domain;
}

property::generic& base::node::get_property(const std::string& name_in)
{
    auto result = properties.find(name_in);

    if (result == properties.end()) {
        system_fault("no property existed with name: ", name_in);
    }

    return *result->second;
}

std::string fully_qualify_property_name(const std::string& name_in)
{
    if (name_in.find(":") == std::string::npos) {
        return std::string("config:") + name_in;
    }

    return name_in;
}

std::string base::node::peek(const std::string& name_in)
{
    auto lock = make_lock();
    auto name = fully_qualify_property_name(name_in);
    return get_property(name).get();
}

const std::map<std::string, property::generic *>& base::node::get_properties()
{
    return properties;
}

property::generic& base::node::add_property(const std::string& name_in, const property::value_type& type_in)
{
    // FIXME why doesn't emplace work?
    auto new_property = new property::generic(name_in, type_in);
    properties[new_property->name] = new_property;
    return *new_property;
}

property::generic& base::node::add_property(const std::string& name_in, property::generic * property_in)
{
    properties[name_in] = property_in;
    return *property_in;
}

void base::node::init()
{

}

void base::node::activate()
{
    audio.activate();

    handle_activate();

    reset();
}

void base::node::run()
{
    auto lock = make_lock();
    handle_run();
    reset();
}

void base::node::handle_run()
{
    audio.notify();
}

void base::node::handle_ready()
{
    domain->add_ready_node(this);
}

void base::node::reset()
{
    audio.reset();
}

bool base::node::is_ready()
{
    return audio.is_ready();
}

chain::chain(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base::node(name_in, domain_in)
{ }

void chain::handle_activate()
{ }

} // namespace node

} // namespace pulsar

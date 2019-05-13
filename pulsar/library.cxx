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

#include <mutex>

#include <pulsar/library.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace library {

static mutex_type node_to_factory_mutex;
static std::map<string_type, node_factory_type> node_to_factory;

static mutex_type daemon_to_factory_mutex;
static std::map<string_type, daemon_factory_type> daemon_to_factory;

void register_node_factory(const string_type& name_in, node_factory_type factory_in)
{
    auto lock = pulsar_get_lock(node_to_factory_mutex);

    auto result = node_to_factory.find(name_in);

    if (result != node_to_factory.end()) {
        system_fault("a node factory was already registered for class name ", name_in);
    }

    node_to_factory[name_in] = factory_in;
}

node::base * make_node(const string_type& class_name_in, const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    auto lock = pulsar_get_lock(node_to_factory_mutex);

    auto result = node_to_factory.find(class_name_in);

    if (result == node_to_factory.end()) {
        system_fault("could not find a node factory for class name ", class_name_in);
    }

    auto factory = result->second;
    return factory(name_in, domain_in);
}

void register_daemon_factory(const string_type& class_name_in, daemon_factory_type factory_in)
{
    auto lock = pulsar_get_lock(daemon_to_factory_mutex);

    auto result = daemon_to_factory.find(class_name_in);

    if (result != daemon_to_factory.end()) {
        system_fault("attempt to double register a daemon with class: " + class_name_in);
    }

    daemon_to_factory[class_name_in] = factory_in;
}

std::shared_ptr<daemon::base> make_daemon(const string_type& class_name_in, const string_type& name_in)
{
    auto lock = pulsar_get_lock(daemon_to_factory_mutex);

    auto result = daemon_to_factory.find(class_name_in);

    if (result == daemon_to_factory.end()) {
        system_fault("could not find a daemon with class " + class_name_in);
    }

    auto factory = result->second;
    return factory(name_in);
}

} // namespace library

} // pulsar

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

#include "library.h"
#include "system.h"

namespace pulsar {

namespace library {

using mutex_type = std::mutex;
using lock_type = std::unique_lock<mutex_type>;

static mutex_type class_to_factory_mutex;
static std::map<std::string, node_factory_type> class_to_factory;

void register_node_factory(const std::string& name_in, node_factory_type factory_in)
{
    lock_type lock(class_to_factory_mutex);

    auto result = class_to_factory.find(name_in);

    if (result != class_to_factory.end()) {
        system_fault("a factory was already registered for class name ", name_in);
    }

    class_to_factory[name_in] = factory_in;
}

node::base::node * make_node(const std::string& class_name_in, const std::string& name_in, std::shared_ptr<domain> domain_in)
{
    lock_type lock(class_to_factory_mutex);

    auto result = class_to_factory.find(class_name_in);

    if (result == class_to_factory.end()) {
        system_fault("could not find a factory for class name ", class_name_in);
    }

    auto factory = result->second;
    return factory(name_in, domain_in);
}

} // namespace library

} // pulsar

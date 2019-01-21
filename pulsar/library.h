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

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <pulsar/domain.h>
#include <pulsar/node.h>

namespace pulsar {

namespace library {

using node_factory_type = std::function<node::base::node * (const string_type&, std::shared_ptr<domain>)>;

void register_node_factory(const string_type& name_in, node_factory_type factory_in);
node::base::node * make_node(const string_type& class_name_in, const string_type& name_in, std::shared_ptr<domain> domain_in);

} // namespace library

} // namespace pulsar

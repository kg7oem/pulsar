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

#include <memory>

#include <pulsar/domain.forward.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>

namespace pulsar {

namespace config {

class domain;
class file;

pulsar::node::base * make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);
std::shared_ptr<pulsar::domain> make_domain(std::shared_ptr<pulsar::config::domain> domain_info_in);
std::map<string_type, pulsar::node::base *> make_nodes(std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);


} // namespace config

} // namespace pulsar

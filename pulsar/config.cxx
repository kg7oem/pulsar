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

#include "config.h"
#include "logging.h"

namespace pulsar {

namespace config {

file::file(const std::string& path_in)
: path(path_in)
{ }

void file::open()
{
    yaml_root = YAML::LoadFile(path);

    validate();
}

void file::validate()
{
    assert(yaml_root["domains"]);
    auto domain_root = yaml_root["domains"];
    assert(domain_root.IsMap());

    for(auto i : domain_root) {
        validate_domain_entry(i.second);
    }
}

void file::validate_domain_entry(const YAML::Node& domain_in)
{
    assert(domain_in.IsMap());
    assert(domain_in["config"].IsMap());
    assert(domain_in["nodes"].IsSequence());
}

void file::validate_node(const YAML::Node&)
{

}

const YAML::Node file::get_domains()
{
    return yaml_root["domains"];
}

const YAML::Node file::get_domain(const std::string& name_in)
{
   auto domains = get_domains();
   assert(domains[name_in]);
   return domains[name_in];
}

} // namespace configfile

} // namespace pulsar

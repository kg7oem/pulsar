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
#include "system.h"

namespace pulsar {

namespace config {

file::file(const std::string& path_in)
: path(path_in)
{ }

void file::open()
{
    yaml_root = YAML::LoadFile(path);
    parse();
}

void file::parse()
{
    if (yaml_root["domain"] && yaml_root["domains"]) {
        system_fault("config file must have one domain or one domains section but not both");
    } else if (! yaml_root["domain"] && ! yaml_root["domains"]) {
        system_fault("config file must have a domain or a domains section");
    }
}

std::vector<std::string> file::get_domain_names()
{
    std::vector<std::string> retval;


    if (yaml_root["domain"]) {
        retval.push_back("main");
    } else if (yaml_root["domains"]) {
        system_fault("can't yet handle a domains section");
    }

    return retval;
}

std::shared_ptr<domain> file::get_domain(const std::string& name_in)
{
    if (name_in == "main" && yaml_root["domain"]) {
        return domain::make(name_in, yaml_root["domain"]);
    }

    auto domains = yaml_root["domains"];
    if (! domains[name_in]) {
        system_fault("could not find a domain named ", name_in);
    }

    return domain::make(name_in, domains[name_in]);
}

domain::domain(const std::string name_in, const YAML::Node& yaml_in)
: yaml_root(yaml_in), name(name_in)
{ }

const YAML::Node domain::get_config()
{
    assert(yaml_root["config"]);
    return yaml_root["config"];
}

const YAML::Node domain::get_nodes()
{
    assert(yaml_root["nodes"]);
    return yaml_root["nodes"];
}

} // namespace configfile

} // namespace pulsar

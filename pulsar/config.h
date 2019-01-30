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

#include <map>
#include <memory>
#include <yaml-cpp/yaml.h>

#include <pulsar/config.forward.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>

namespace pulsar {

struct domain;

namespace config {

class file : public std::enable_shared_from_this<file> {
    private:
    YAML::Node yaml_root;
    void open();
    void parse();

    public:
    const string_type path;
    file(const string_type& path_in);
    template <typename... Args>
    static std::shared_ptr<file> make(Args&&... args) {
        auto new_file = std::make_shared<file>(args...);
        new_file->open();
        return new_file;
    }
    std::vector<string_type> get_domain_names();
    std::shared_ptr<domain> get_domain(const string_type& name_in = "main");
    const YAML::Node get_templates();
    YAML::Node get_template(const string_type& name_in);
    const YAML::Node get_chains();
    const YAML::Node get_chain(const string_type& name_in);
    const YAML::Node get_engine();
    const YAML::Node get_daemons();
};

class domain : public std::enable_shared_from_this<domain> {
    const YAML::Node yaml_root;
    std::shared_ptr<file> parent;

    public:
    const string_type name;
    domain(const string_type name_in, const YAML::Node& yaml_in, std::shared_ptr<file> parent_in);
    template <typename... Args>
    static std::shared_ptr<domain> make(Args&&... args) {
        auto new_domain = std::make_shared<domain>(args...);
        return new_domain;
    }
    std::shared_ptr<file> get_parent();
    const YAML::Node get_config();
    const YAML::Node get_nodes();
};

} // namespace configfile

} // namespace pulsar

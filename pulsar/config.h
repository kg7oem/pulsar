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
#include <yaml-cpp/yaml.h>

namespace pulsar {

struct domain;

namespace node {

namespace base {

struct node;

} // namespace base

} // namespace node

namespace config {

struct domain;

std::shared_ptr<pulsar::domain> make_domain(std::shared_ptr<pulsar::config::domain> domain_info_in);
std::vector<pulsar::node::base::node *> make_nodes(std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);

class file : public std::enable_shared_from_this<file> {
    private:
    YAML::Node yaml_root;
    void open();
    void parse();

    public:
    const std::string path;
    file(const std::string& path_in);
    template <typename... Args>
    static std::shared_ptr<file> make(Args&&... args) {
        auto new_file = std::make_shared<file>(args...);
        new_file->open();
        return new_file;
    }
    std::vector<std::string> get_domain_names();
    std::shared_ptr<domain> get_domain(const std::string& name_in = "main");
};

class domain : public std::enable_shared_from_this<domain> {
    const YAML::Node yaml_root;

    public:
    const std::string name;
    domain(const std::string name_in, const YAML::Node& yaml_in);
    template <typename... Args>
    static std::shared_ptr<domain> make(Args&&... args) {
        auto new_domain = std::make_shared<domain>(args...);
        return new_domain;
    }

    const YAML::Node get_config();
    const YAML::Node get_nodes();
};

} // namespace configfile

} // namespace pulsar

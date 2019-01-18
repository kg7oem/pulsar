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

namespace config {

class file : public std::enable_shared_from_this<file> {
    private:
    YAML::Node yaml_root;
    void validate_domain_entry(const YAML::Node& domain_in);
    void validate_node(const YAML::Node& node_in);

    public:
    const std::string path;
    file(const std::string& path_in);
    template <typename... Args>
    static std::shared_ptr<file> make(Args&&... args) {
        auto new_file = std::make_shared<file>(args...);
        new_file->open();
        return new_file;
    }
    void open();
    void validate();
    const YAML::Node get_domains();
    const YAML::Node get_domain(const std::string& name_in);
};

} // namespace configfile

} // namespace pulsar

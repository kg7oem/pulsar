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

#include <yaml-cpp/yaml.h>

#include <pulsar/types.h>

namespace pulsar {

namespace daemon {

struct base {
    const string_type name;

    base(const string_type& name);
    virtual ~base() = default;
    virtual void init(const YAML::Node& yaml_in) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

} // namespace daemon

} // namespace pulsar

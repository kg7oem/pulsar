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
#include <string>

#include <pulsar/config.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>

namespace pulsar {

namespace property {

enum class value_type { unknown, size, integer, real, string };

union value_container {
    size_type size;
    integer_type integer;
    real_type real;
    string_type * string;
};

class storage : public std::enable_shared_from_this<storage> {
    storage(const storage&) = delete;

    protected:
    value_container value;

    public:
    const value_type type = value_type::unknown;
    storage(const value_type& type_in);
    virtual ~storage();
    string_type get();
    void set(const double& value_in);
    void set(const string_type& value_in);
    void set(const YAML::Node& value_in);
    size_type& get_size();
    void set_size(const size_type& size_in);
    integer_type& get_integer();
    void set_integer(const integer_type& integer_in);
    void set_real(const real_type& real_in);
    real_type& get_real();
    string_type& get_string();
    void set_string(const string_type& string_in);
};

class property {
    protected:
    node::base * parent;

    public:
    const string_type name;
    const std::shared_ptr<storage> value;
    property(node::base * parent_in, const string_type& name_in, const value_type type_in);
    property(node::base * parent_in, const string_type& name_in, std::shared_ptr<storage> storage_in);
};

} // namespace property

} // namespace pulsar

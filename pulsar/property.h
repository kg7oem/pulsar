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

#include <string>

#include "config.h"
#include "system.h"

namespace pulsar {

namespace property {

enum class value_type { unknown, size, integer, real, string };

union value_container {
    size_type size;
    integer_type integer;
    real_type real;
    std::string * string;
};

class generic {
    protected:
    value_container value;

    public:
    const std::string name;
    const value_type type = value_type::unknown;
    generic(const std::string& name_in, const value_type& type_in);
    virtual ~generic();
    std::string get();
    // void set(const size_type& value_in);
    void set(const double& value_in);
    void set(const std::string& value_in);
    void set(const YAML::Node& value_in);
    size_type& get_size();
    void set_size(const size_type& size_in);
    integer_type& get_integer();
    void set_integer(const integer_type& integer_in);
    void set_real(const real_type& real_in);
    real_type& get_real();
    std::string& get_string();
    void set_string(const std::string& string_in);
};

// struct integer : public generic {
//     integer(const std::string& name_in);
// };

// struct string : public generic {
//     string(const std::string& name_in);
// };

} // namespace property

} // namespace pulsar

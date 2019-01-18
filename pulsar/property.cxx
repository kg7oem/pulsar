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

#include <cstdlib>

#include "logging.h"
#include "property.h"
#include "system.h"

namespace pulsar {

namespace property {

generic::generic(const std::string& name_in, const value_type& type_in)
: name(name_in), type(type_in)
{
    switch(type) {
        case value_type::unknown: system_fault("can not specify unknown as a parameter type");
        case value_type::size: value.size = 0;
        case value_type::integer: value.integer = 0;
        case value_type::real: value.real = 0;
        case value_type::string: new std::string;
    }
}

generic::~generic()
{
    if (type == value_type::string && value.string != nullptr) {
        delete(value.string);
        value.string = nullptr;
    }
}

std::string generic::get()
{
    switch(type) {
        case value_type::unknown: system_fault("parameter type was not known");
        case value_type::size: return std::to_string(value.size);
        case value_type::integer: return std::to_string(value.integer);
        case value_type::real: return std::to_string(value.real);
        case value_type::string: return *value.string;
    }

    system_fault("should never get out of switch statement");
}

void generic::set(const double& value_in)
{
    switch(type) {
        case value_type::unknown: system_fault("parameter type was not known"); return;
        case value_type::size: set_size(value_in); return;
        case value_type::integer: set_integer(value_in); return;
        case value_type::real: set_real(value_in); return;
        case value_type::string: system_fault("unsupported for string type");
    }

    system_fault("should never get out of switch statement");
}

// void generic::set(const size_type& value_in)
// {
//     set_size(value_in);
// }

// void generic::set(const integer_type& value_in)
// {
//     set_integer(value_in);
// }

// void generic::set(const real_type& value_in)
// {
//     set_real(value_in);
// }

void generic::set(const std::string& value_in)
{
    auto c_str = value_in.c_str();

    switch(type) {
        case value_type::unknown: system_fault("parameter type was not known");
        case value_type::size: value.size = std::strtoul(c_str, nullptr, 0); return;
        case value_type::integer: value.integer = std::atoi(c_str); return;
        case value_type::real: value.real = std::strtof(c_str, nullptr); return;
        case value_type::string: *value.string = value_in; return;
    }

    system_fault("should never get out of switch statement");
}

size_type& generic::get_size()
{
    if (type != value_type::size) {
        system_fault("parameter is not of type: size");
    }

    return value.size;
}

void generic::set_size(const size_type& size_in)
{
    if (type != value_type::size) {
        system_fault("parameter is not of type: size");
    }

    value.size = size_in;
}

integer_type& generic::get_integer()
{
    if (type != value_type::integer) {
        system_fault("parameter is not of type: integer");
    }

    return value.integer;
}

void generic::set_integer(const integer_type& integer_in)
{
    if (type != value_type::integer) {
        system_fault("parameter is not of type: integer");
    }

    value.integer = integer_in;
}

void generic::set_real(const real_type& real_in)
{
    if (type != value_type::real) {
        system_fault("parameter is not of type: real");
    }

    value.real = real_in;
}

real_type& generic::get_real()
{
    if (type != value_type::real) {
        system_fault("parameter is not of type: real");
    }

    return value.real;
}

void generic::set_string(const std::string& string_in)
{
    if (type != value_type::string) {
        system_fault("parameter is not of type: string");
    }

    *value.string = string_in;
}

std::string& generic::get_string()
{
    if (type != value_type::string) {
        system_fault("parameter is not of type: string");
    }

    return *value.string;
}

// integer::integer(const std::string& name_in)
// : generic(name_in, value_type::integer)
// { }

// integer_type& integer::get()
// {
//     return get_integer();
// }

// void integer::set(const integer_type& value_in)
// {
//     set_integer(value_in);
// }

// string::string(const std::string& name_in)
// : generic(name_in, value_type::string)
// { }

// std::string& string::get()
// {
//     return get_string();
// }

// void string::set(const std::string& value_in)
// {
//     set_string(value_in);
// }

} // namespace parameter

} // namespace pulsar

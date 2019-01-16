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

// g++ 6.3.0 as it comes in debian/stretch does not support maybe_unused
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED [[ maybe_unused ]]
#endif

// #define system_fault(...) pulsar::system::fault__func(__FILE__, __LINE__, __PRETTY_FUNCTION__, oemros::vaargs_to_string(__VA_ARGS__))
// #define system_panic(...) pulsar::system::panic__func(__FILE__, __LINE__, __PRETTY_FUNCTION__, oemros::vaargs_to_string(__VA_ARGS__))

namespace pulsar {

namespace system {

const std::string& get_boost_version();

// [[noreturn]] void system_fault__func(const char* file_in, int line_in, const char* function_in, const std::string& message_in);
// [[noreturn]] void system_panic__func(const char* file_in, int line_in, const char* function_in, const std::string& message_in);

} // namespace system

} // namespace pulsar

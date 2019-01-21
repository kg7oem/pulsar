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

#include <sstream>
#include <string>

#include "logjam.h"

// g++ 6.3.0 as it comes in debian/stretch does not support maybe_unused
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED [[ maybe_unused ]]
#endif

#define system_fault(...) pulsar::system::fault(__FILE__, __LINE__, __PRETTY_FUNCTION__, logjam::vaargs_to_string(__VA_ARGS__))

namespace pulsar {

using integer_type = int;
using real_type = float;
using size_type = unsigned long;
using string_type = std::string;

using sample_type = real_type;

namespace system {

using alive_handler_type = std::function<void (void *)>;

void bootstrap();
const std::string& get_boost_version();
void register_alive_handler(alive_handler_type cb_in, void * arg_in = nullptr);

[[noreturn]] void fault(const char* file_in, int line_in, const char* function_in, const std::string& message_in);

} // namespace system

} // namespace pulsar

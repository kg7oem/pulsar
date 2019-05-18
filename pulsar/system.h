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

#include <chrono>
#include <functional>
#include <sstream>
#include <string>

#include <pulsar/types.h>
#include <pulsar/util.h>

// g++ 6.3.0 as it comes in debian/stretch does not support maybe_unused
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED [[ maybe_unused ]]
#endif

#ifdef NDEBUG
#define NDEBUG_UNUSED UNUSED
#else
#define NDEBUG_UNUSED
#endif

#define CONFIG_LOCK_ASSERT
#define CONFIG_LOCK_LOGGING

#define system_fault(...) pulsar::system::fault(__FILE__, __LINE__, __PRETTY_FUNCTION__, pulsar::util::to_string(__VA_ARGS__))

namespace pulsar {

namespace system {

using alive_handler_type = std::function<void (void *)>;

[[noreturn]] void fault(const char* file_in, int line_in, const char* function_in, const string_type& message_in);

void bootstrap(const size_type num_threads_in);
void shutdown();
void wait_stopped();
const string_type& get_boost_version();
void register_alive_handler(alive_handler_type cb_in, void * arg_in = nullptr);
void enable_memory_logging(const duration_type& max_age_in, const string_type& level_name_in);

} // namespace system

} // namespace pulsar

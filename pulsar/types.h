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

#include <boost/pool/pool_alloc.hpp>
#include <chrono>
#include <sstream>
#include <string>

#define PULSAR_STRING_POOLS

namespace pulsar {

// The boost pool allocator and fast pool allocator are both
// thread safe.
template <typename T>
using pool_allocator_type = boost::pool_allocator<T>;

using char_type = char;
using duration_type = std::chrono::milliseconds;
using integer_type = int;
using real_type = float;
using size_type = unsigned long;

#ifdef PULSAR_STRING_POOLS
// FIXME how to convert from basic_string to std::string so this can work transparently
// when something is expecting a std::string?
using string_type = std::basic_string<char_type, std::char_traits<char_type>, pool_allocator_type<char_type>>;
using stringstream_type = std::basic_stringstream<char_type, std::char_traits<char_type>, pool_allocator_type<char_type>>;
#else // PULSAR_STRING_POOLS
using string_type = std::basic_string<char, std::char_traits<char>>;
using stringstream_type = std::basic_stringstream<char_type, std::char_traits<char_type>>;
#endif // PULSAR_STRING_POOLS

// FIXME rename to audio_sample_type or move
// into pulsar::audio::sample_type <-- probably best
using sample_type = real_type;

} // namespace pulsar

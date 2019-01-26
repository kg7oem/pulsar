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
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace pulsar {

using duration_type = std::chrono::milliseconds;
using integer_type = int;
using real_type = float;
using size_type = unsigned long;
using string_type = std::string;

using condition_type = std::condition_variable;
using mutex_type = std::mutex;
using lock_type = std::unique_lock<mutex_type>;
using thread_type = std::thread;

// FIXME rename to audio_sample_type or move
// into pulsar::audio::sample_type <-- probably best
using sample_type = real_type;

} // namespace pulsar

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

#include <atomic>

#include <pulsar/debug.h>

namespace pulsar {

namespace debug {

static std::atomic<bool> lock_watchdogs_enabled_flag = ATOMIC_VAR_INIT(false);

bool get_lock_watchdogs_enabled() {
    return lock_watchdogs_enabled_flag.load();
}

void set_lock_watchdogs_enabled(const bool enabled_in)
{
    lock_watchdogs_enabled_flag.store(enabled_in);
}

} // namespace debug

} // namespace pulsar


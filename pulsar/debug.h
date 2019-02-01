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

#include <pulsar/logging.h>
#include <pulsar/system.h>
#include <pulsar/types.h>
#include <pulsar/util.h>

#define LOCK_LOGGING

#define debug_get_lock(mutex) pulsar::debug::get_lock_wrapper(PULSAR_LOG_LOCK_NAME, logjam::loglevel::trace, __PRETTY_FUNCTION__, __FILE__, __LINE__, mutex, #mutex)
#define debug_relock(lock) pulsar::debug::relock_wrapper(PULSAR_LOG_LOCK_NAME, logjam::loglevel::trace, __PRETTY_FUNCTION__, __FILE__, __LINE__, lock, #lock)

namespace pulsar {

namespace debug {

using namespace std::chrono_literals;

template <typename T>
std::unique_lock<T> get_lock_wrapper(const string_type& logname_in, const logjam::loglevel& level_in, const char *function_in, const char *path_in, const int& line_in, T& mutex_in, const string_type& name_in) {
    thread_local bool went_recursive = false;

    if (went_recursive) system_fault("aborting because of recursion");

    went_recursive = true;

#ifdef LOCK_LOGGING
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "getting lock for ", name_in);
    auto lock = std::unique_lock<T>(mutex_in);
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "got lock for ", name_in);
#endif

    went_recursive = false;

    return lock;
}

template <typename T>
void relock_wrapper(const string_type& logname_in, const logjam::loglevel& level_in, const char *function_in, const char *path_in, const int& line_in, T& lock_in, const string_type& name_in) {
    thread_local bool went_recursive = false;

    if (went_recursive) system_fault("aborting because of recursion");

    went_recursive = true;

#ifdef LOCK_LOGGING
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "relocking lock: ", name_in);
    lock_in.lock();
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "finished relocking: ", name_in);
#endif

    went_recursive = false;
}

} // namespace debug

} // namespace pulsar

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

// FIXME this can't stay
#include <pulsar/async.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>
#include <pulsar/types.h>
#include <pulsar/util.h>

#define PULSAR_DEFAULT_LOCK_TIMEOUT 250ms

#define debug_get_lock(...) pulsar::debug::get_lock_wrapper(PULSAR_LOG_LOCK_NAME, logjam::loglevel::trace, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

namespace pulsar {

namespace debug {

using namespace std::chrono_literals;

#define LOCK_LOGGING
#undef LOCK_WATCHDOGS
#define LOCK_WATCHDOG_DEFAULT 100ms

template <typename T>
std::unique_lock<T> get_lock_wrapper(const std::string& logname_in, const logjam::loglevel& level_in, const char *function_in, const char *path_in, const int& line_in, T& mutex_in, UNUSED const duration_type timeout_in = LOCK_WATCHDOG_DEFAULT) {
#ifdef LOCK_WATCHDOGS
    thread_local bool watchdog_recursive = false;
    std::shared_ptr<async::watchdog> lock_watchdog;

    // FIXME check for 0 timeout and skip
    if (! watchdog_recursive && async::is_online()) {
        watchdog_recursive = true;

        log_trace("creating a lock watchdog");
        auto message = util::to_string("lock timeout at ", path_in, ":", line_in);
        lock_watchdog = async::watchdog::make(timeout_in, message);
        // FIXME this will crash asio
        // lock_watchdog->start();

        watchdog_recursive = false;
    }
#endif

#ifdef LOCK_LOGGING
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "getting lock");
    auto lock = std::unique_lock<T>(mutex_in);
    logjam::send_vargs_logevent(logname_in, level_in, function_in, path_in, line_in, "got lock");
#endif

#ifdef LOCK_WATCHDOGS
    if (lock_watchdog != nullptr) lock_watchdog->stop();
#endif

    return lock;
}

} // namespace debug

} // namespace pulsar

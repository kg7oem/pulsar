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

#include <pthread.h>
#include <string.h>

#include <pulsar/logging.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>
#include <pulsar/types.h>

#define NO_FUNCTION "(none)"
#define NO_FILE "(none)"
#define NO_LINE 0

#define FOREIGN_FUNCTION "(foreign)"
#define FOREIGN_FILE "(foreign)"
#define FOREIGN_LINE 0

namespace pulsar {

namespace thread {

// from https://stackoverflow.com/a/31652324
void set_realtime_priority(thread_type& thread_in, const rt_priorty& priority_in)
{
    sched_param sch_params;
    sch_params.sched_priority = static_cast<int>(priority_in);

    if (auto error = pthread_setschedparam(thread_in.native_handle(), SCHED_RR, &sch_params)) {
        log_error("could not set thread to realtime priority: ", strerror(error));
    }
}

lock_type get_lock(UNUSED const char *function_in, UNUSED const char *path_in, UNUSED const int& line_in, mutex_type& mutex_in, UNUSED const string_type& name_in)
{

#ifdef CONFIG_LOCK_LOGGING
    logjam::send_vargs_logevent(PULSAR_LOG_LOCK_NAME, PULSAR_LOG_LOCK_LEVEL, function_in, path_in, line_in, "getting lock for ", name_in);
#endif

    lock_type lock(mutex_in);

#ifdef CONFIG_LOCK_LOGGING
    logjam::send_vargs_logevent(PULSAR_LOG_LOCK_NAME, PULSAR_LOG_LOCK_LEVEL, function_in, path_in, line_in, "got lock for ", name_in);
#endif

    return lock;
}

void lock_block(UNUSED const char *function_in, UNUSED const char *path_in, UNUSED const int& line_in, mutex_type& mutex_in, const std::function<void ()>& block_in)
{
    auto lock = pulsar_get_lock(mutex_in);
    block_in();
}

#ifdef CONFIG_LOCK_ASSERT

debug_mutex::debug_mutex()
{
    reset();
}

debug_mutex::~debug_mutex()
{ }

void debug_mutex::reset()
{
    available_flag = true;
    owner_function = NO_FUNCTION;
    owner_file = NO_FILE;
    owner_line = NO_LINE;
}

void debug_mutex::lock()
{
    handle_lock(FOREIGN_FUNCTION, FOREIGN_FILE, FOREIGN_LINE);
}

void debug_mutex::lock(const char *function_in, const char *path_in, const int& line_in)
{
    handle_lock(function_in, path_in, line_in);
}

void debug_mutex::handle_lock(const char *function_in, const char *path_in, const int& line_in)
{
    auto&& thread_id = std::this_thread::get_id();
    std::unique_lock lock(our_mutex);

    waiters[thread_id] = pulsar::util::to_string(owner_file, ":", owner_line, " ", owner_function, "()");
    available_condition.wait(lock, [this]{ return available_flag; });
    NDEBUG_UNUSED auto erased = waiters.erase(thread_id);
    assert(erased == 1);

    available_flag = false;
    owner_thread = std::this_thread::get_id();
    owner_function = function_in;
    owner_file = path_in;
    owner_line = line_in;

    return;
}

void debug_mutex::unlock()
{
    std::unique_lock lock(our_mutex);

    if (available_flag) {
        system_fault("thread ", std::this_thread::get_id(), " tried to unlock a mutex not owned by any thread");
    }

    if (owner_thread != std::this_thread::get_id()) {
        system_fault("thread ",  std::this_thread::get_id(), " tried to unlock a mutex owned by another thread ", owner_thread);
    }

    reset();

    available_condition.notify_one();
}

bool debug_mutex::is_owned_by(const std::thread::id thread_id_in)
{
    std::unique_lock lock(our_mutex);
    return owner_thread == thread_id_in;
}

#endif

} // namespace thread

} // namespace pulsar

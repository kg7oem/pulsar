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

debug_mutex::~debug_mutex()
{ }

void debug_mutex::lock()
{
    std::unique_lock lock(our_mutex);

    available_condition.wait(lock, [this]{ return available_flag; });

    available_flag = false;
    owner = std::this_thread::get_id();

    return;
}

void debug_mutex::unlock()
{
    std::unique_lock lock(our_mutex);

    if (available_flag) {
        system_fault("thread ", std::this_thread::get_id(), " tried to unlock a mutex not owned by any thread");
    }

    if (owner != std::this_thread::get_id()) {
        system_fault("thread ",  std::this_thread::get_id(), " tried to unlock a mutex owned by thread ", owner);
    }

    available_flag = true;
    available_condition.notify_one();
}

bool debug_mutex::is_owned_by(const std::thread::id thread_id_in)
{
    std::unique_lock lock(our_mutex);
    return owner == thread_id_in;
}

#endif

} // namespace thread

} // namespace pulsar

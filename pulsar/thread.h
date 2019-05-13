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

#include <cassert>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <pulsar/system.h>
#include <pulsar/thread.forward.h>

#define pulsar_get_lock(mutex) pulsar::thread::get_lock(__PRETTY_FUNCTION__, __FILE__, __LINE__, mutex, #mutex)
#define pulsar_lock_block(mutex, block) pulsar::thread::lock_block(__PRETTY_FUNCTION__, __FILE__, __LINE__, mutex, [&]() -> void block)

namespace pulsar {

#ifdef CONFIG_LOCK_ASSERT
#define assert_mutex_owner(mutex) assert(mutex.is_owned_by(std::this_thread::get_id()))

using mutex_type = pulsar::thread::debug_mutex;
using condition_type = std::condition_variable_any;
#else
#define assert_mutex_owner(mutex)

using mutex_type = std::mutex;
using condition_type = std::condition_variable;
#endif

using lock_type = std::unique_lock<mutex_type>;

template <typename T> using promise_type = std::promise<T>;
using thread_type = std::thread;

namespace thread {

enum class rt_priorty : int {
    lowest = 1,
    normal = 5,
    highest = 10,
};

#ifdef CONFIG_LOCK_ASSERT
class debug_mutex {
    protected:
    std::mutex our_mutex;
    bool available_flag = true;
    std::condition_variable available_condition;
    std::thread::id owner;

    public:
    virtual ~debug_mutex();
    virtual void lock();
    virtual void unlock();
    virtual bool is_owned_by(const std::thread::id thread_id_in);
};
#endif

void set_realtime_priority(thread_type& thread_in, const rt_priorty& priority_in);
lock_type get_lock(UNUSED const char *function_in, UNUSED const char *path_in, UNUSED const int& line_in, mutex_type& mutex_in, UNUSED const string_type& name_in);
void lock_block(UNUSED const char *function_in, UNUSED const char *path_in, UNUSED const int& line_in, mutex_type& mutex_in, const std::function<void ()>& block_in);

} // namespace thread

} // namespace pulsar

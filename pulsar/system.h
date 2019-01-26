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
#include <iostream>
#include <map>
#include <iostream>
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

#define PULSAR_ALLOCATOR_BLOCK_SIZE 128
#define PULSAR_ALLOCATOR_MAX_ITEMS 20

#define system_fault(...) pulsar::system::fault(__FILE__, __LINE__, __PRETTY_FUNCTION__, pulsar::util::to_string(__VA_ARGS__))

namespace pulsar {

namespace system {

using alive_handler_type = std::function<void (void *)>;

[[noreturn]] void fault(const char* file_in, int line_in, const char* function_in, const string_type& message_in);

enum class rt_priorty : int {
    lowest = 1,
    normal = 5,
    highest = 10,
};

// this should use std::size_t instead of the pulsar size_type
template <class T>
struct allocator {
    using value_type = T;
    using pool_type = std::map<std::size_t, std::vector<void *>>;

    pool_type &pool;
    mutex_type pool_mutex;

    allocator(pool_type &pool_in)
    : pool(pool_in)
    { }

    template <class U> constexpr allocator(const allocator<U>& from_in) noexcept
    : pool(from_in.pool)
    { }

    template<class U, class... Args>
    void construct(U * p, Args&&... args)
    {
        std::cout << "allocator: construct object" << std::endl;
        // placement new operator
        new (p) U(args...);
        return;
    }

    template<class U>
    void destroy(U * p)
    {
        std::cout << "allocator: destroy object" << std::endl;
        // from https://stackoverflow.com/a/3763887
        // Do not release the memory allocated by a call to the placement new operator using
        // the delete keyword. You will destroy the object by calling the destructor directly.
        // p->~Foo();
        p->~T();
    }

    std::size_t bytes_fit_to_blocks(std::size_t num_things_in)
    {
        auto requested_bytes = num_things_in * sizeof(T);
        auto fit_bytes = requested_bytes + requested_bytes % PULSAR_ALLOCATOR_BLOCK_SIZE;
        std::cout << "allocator: requested = " << requested_bytes << "; got " << fit_bytes << std::endl;
        return fit_bytes;
    }

    T* allocate(std::size_t num_things_in) {
        // lock_type lock(pool_mutex);

        std::size_t size = bytes_fit_to_blocks(num_things_in);
        std::cout << "allocator: allocate memory: " << size << std::endl;
        auto p = std::malloc(size);
        return static_cast<T *>(p);
    }

    void deallocate(T * p, std::size_t UNUSED num_things_in) noexcept
    {
        // lock_type lock(pool_mutex);

        UNUSED std::size_t size = bytes_fit_to_blocks(num_things_in);
        std::cout << "allocator: free memory: " << size << std::endl;
        std::free(p);
    }
};

template <class T, class U>
bool operator==(const allocator<T>&, const allocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const allocator<T>&, const allocator<U>&) { return false; }

void bootstrap();
const std::string& get_boost_version();
void register_alive_handler(alive_handler_type cb_in, void * arg_in = nullptr);
void enable_memory_logging(const duration_type& max_age_in, const string_type& level_name_in);
void set_realtime_priority(thread_type& thread_in, const rt_priorty& priority_in);

} // namespace system

} // namespace pulsar

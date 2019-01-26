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

struct allocator_pool {
    std::map<std::size_t, std::vector<void *>> pointers_by_size;
    std::mutex mutex;

    allocator_pool();
    ~allocator_pool();
};

// this should use std::size_t instead of the pulsar size_type
template <class T>
struct allocator {
    using value_type = T;

    allocator_pool &pool;

    allocator(allocator_pool &pool_in)
    : pool(pool_in)
    { }

    template <class U> constexpr allocator(const allocator<U>& from_in) noexcept
    : pool(from_in.pool)
    { }

    template<class U, class... Args>
    void construct(U * p, Args&&... args)
    {
        // std::cout << "allocator: construct object" << std::endl;
        // placement new operator
        new (p) U(args...);
        return;
    }

    template<class U>
    void destroy(U * p)
    {
        // std::cout << "allocator: destroy object" << std::endl;
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
        // std::cout << "allocator: requested = " << requested_bytes << "; got " << fit_bytes << std::endl;
        return fit_bytes;
    }

    T* allocate(std::size_t num_things_in) {
        std::size_t size = bytes_fit_to_blocks(num_things_in);

        lock_type lock(pool.mutex);
        auto found = pool.pointers_by_size.find(size);
        void * p = nullptr;

        if (found == pool.pointers_by_size.end()) {
            std::cout << "allocator: no entries found for size " << size << std::endl;
            p = std::malloc(size);
        } else if (found->second.size() == 0) {
            std::cout << "allocator: free list was empty for size " << size << std::endl;
            p = std::malloc(size);
        } else {
            std::cout << "allocator: getting memory from free list: " << size << std::endl;
            p = found->second.back();
            found->second.pop_back();
        }

        assert(p != nullptr);
        return static_cast<T *>(p);
    }

    void deallocate(T * p, std::size_t num_things_in) noexcept
    {
        auto size = bytes_fit_to_blocks(num_things_in);

        lock_type lock(pool.mutex);

        auto found = pool.pointers_by_size.find(size);

        if (found == pool.pointers_by_size.end()) {
            std::cout << "deallocator: no list available for " << size << std::endl;
        } else {
            std::cout << "deallocator: list was available for " << size << std::endl;
            std::cout << "dealloctor: list size: " << pool.pointers_by_size.size() << std::endl;
        }

        auto free_list = pool.pointers_by_size[size];

        if (free_list.size() > PULSAR_ALLOCATOR_MAX_ITEMS) {
            std::cout << "deallocator: giving memory back to the system: " << size << std::endl;
            std::free(p);
        } else {
            std::cout << "deallocator: putting memory into free list: " << size << std::endl;
            free_list.push_back(p);
        }
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
allocator_pool& get_allocator_pool();

} // namespace system

} // namespace pulsar

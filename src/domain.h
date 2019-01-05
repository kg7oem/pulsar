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

#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "audio.h"
#include "config.h"

namespace pulsar {

class node;

struct domain : public std::enable_shared_from_this<domain> {
    using mutex_type = std::mutex;
    using lock_type = std::unique_lock<mutex_type>;

    private:
    audio::buffer zero_buffer;
    std::vector<std::shared_ptr<node>> nodes;
    std::vector<node *> run_queue;
    mutex_type run_queue_mutex;
    std::condition_variable run_queue_condition;
    std::atomic<size_type> remaining_nodes = ATOMIC_VAR_INIT(0);
    std::vector<std::thread> threads;
    std::condition_variable step_done_condition;
    mutex_type step_done_mutex;
    bool activated = false;
    bool step_done_flag = false;
    lock_type make_step_done_lock();
    lock_type make_run_queue_lock();
    static void be_thread(domain * domain_in);

    public:
    const std::string name;
    const pulsar::size_type sample_rate;
    const pulsar::size_type buffer_size;
    domain(const std::string& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in);
    virtual ~domain();
    audio::buffer& get_zero_buffer();
    void activate(const size_type num_threads_in = 1);
    void reset();
    void step();
    void add_ready_node(node * node_in);
    template<class T, typename... Args>
    std::shared_ptr<T> make_node(Args... args)
    {
        auto new_node = std::make_shared<T>(args..., this->shared_from_this());
        if (activated) {
            new_node->activate();
        }
        nodes.push_back(new_node);
        return new_node;
    }
};

} // namespace pulsar

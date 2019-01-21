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

#include <cassert>

#include "domain.h"
#include "logging.h"
#include "node.h"

namespace pulsar {

domain::domain(const std::string& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in)
: name(name_in), sample_rate(sample_rate_in), buffer_size(buffer_size_in)
{
    // FIXME use mprotect() to set the zero_buffer memory as read-only
    // so it can't be accidently written over
    zero_buffer->init(buffer_size_in);
}

domain::~domain()
{ }

std::shared_ptr<audio::buffer> domain::get_zero_buffer()
{
    return zero_buffer;
}

domain::lock_type domain::make_run_queue_lock()
{
    return lock_type(run_queue_mutex);
}

void domain::activate(const size_type num_threads_in)
{
    if (num_threads_in <= 0) {
        throw std::runtime_error("attempt to activate a domain with invalid number of threads");
    }

    assert(! activated);

    activated = true;

    // FIXME Activate all the nodes before any threads exist to run them
    // so a node is not executed from the ready list before it is
    // activated. Right now if the order is reversed race conditions probably
    // exist during startup.
    //
    // This will probably hold until topology changes are happening while
    // the domain is running. If topology changes are safe then activation
    // should be able to happen in any order.

    for(auto&& node : nodes) {
        node->activate();
    }

    for(size_type i = 0; i < num_threads_in; i++) {
        threads.emplace_back(be_thread, this);
    }
}

void domain::add_ready_node(node::base::node * node_in)
{
    assert(activated);

    auto lock = make_run_queue_lock();

    log_trace("adding ready node: ", node_in->name);

    run_queue.push_back(node_in);
    run_queue_condition.notify_one();
}

void domain::be_thread(domain * domain_in)
{
    while(1) {
        auto lock = domain_in->make_run_queue_lock();
        domain_in->run_queue_condition.wait(lock, [domain_in]{
            return domain_in->run_queue.size() > 0;
        });

        auto ready_node = domain_in->run_queue.front();
        domain_in->run_queue.pop_front();

        lock.unlock();

        log_trace("running node: ", ready_node->name);
        ready_node->execute();
        log_trace("done running node: ", ready_node->name);
    }
}

} // namespace pulsar

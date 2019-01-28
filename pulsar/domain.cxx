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
#include <functional>

#include <pulsar/debug.h>
#include <pulsar/domain.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>

namespace pulsar {

static string_type make_dbus_path(const string_type& domain_name_in)
{

    return util::to_string(PULSAR_DBUS_DOMAIN_PREFIX, domain_name_in);
}

dbus_node::dbus_node(std::shared_ptr<domain> parent_in)
:
    DBus::ObjectAdaptor(dbus::get_connection(), make_dbus_path(parent_in->name)),
    parent(parent_in)
{ }

std::string dbus_node::name()
{
    return parent->name;
}

domain::domain(const string_type& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in)
: name(name_in), sample_rate(sample_rate_in), buffer_size(buffer_size_in)
{
    // FIXME use mprotect() to set the zero_buffer memory as read-only
    // so it can't be accidently written over
    zero_buffer->init(buffer_size_in);
}

domain::~domain()
{
    if (dbus != nullptr) {
        delete dbus;
        dbus = nullptr;
    }
}

void domain::init()
{
    dbus = new dbus_node(this->shared_from_this());
}

std::shared_ptr<audio::buffer> domain::get_zero_buffer()
{
    return zero_buffer;
}

void domain::activate(const size_type num_threads_in)
{
    if (num_threads_in <= 0) {
        system_fault("attempt to activate a domain with invalid number of threads");
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
        threads.emplace_back(std::bind(&domain::be_thread, this));
        thread::set_realtime_priority(threads.back(), thread::rt_priorty::highest);
    }

    // FIXME
    // jackaudio exposed the race condition between start/activate/threads even
    // with the ordering change because it brought it's own thread with it.
    //
    // start() should become a node command and start becomes a part of the lifecycle
    // which will become important when topology changes come around.
    for(auto&& node : nodes) {
        node->start();
    }
}

void domain::add_ready_node(node::base * node_in)
{
    llog_trace({ return pulsar::util::to_string("adding ready node: ", node_in->name); });

    assert(activated);

    auto lock = debug_get_lock(run_queue_mutex);

    run_queue.push_back(node_in);
    run_queue_condition.notify_one();

    log_trace("done adding ready node ", node_in->name);
}

void domain::be_thread()
{
    while(1) {
        auto lock = debug_get_lock(run_queue_mutex);

        run_queue_condition.wait(lock, [this]{ return run_queue.size() > 0; });

        node::base * ready_node;

        // Dequeue strategy needs to be FIFO so any previous
        // task that was scheduled to run is removed before a
        // new one is removed. Topologies exist that are valid
        // but include leafs that would wind up in a list that
        // continues to grow if LIFO is used.
        ready_node = run_queue.front();
        run_queue.pop_front();

        lock.unlock();

        llog_trace({ return pulsar::util::to_string("running node: ", ready_node->name); });
        ready_node->execute();
        llog_trace({ return pulsar::util::to_string("done running node: ", ready_node->name); });
    }
}

} // namespace pulsar

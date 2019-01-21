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

#include <boost/bind.hpp>
#include <exception>
#include <thread>
#include <vector>

#include <pulsar/async.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

namespace pulsar {

namespace async {

using namespace std::chrono_literals;

static boost::asio::io_service boost_io;
static std::vector<thread_type> async_threads;

// boost::asio::io_service& get_global_io()
// {
//     return boost_io;
// }

static void async_thread()
{
    boost_io.run();
    system_fault("Boost io.run() returned");
}

// void init_tick()
// {
//     assert(tick_timer == nullptr);

//     tick_timer = timer::make(0s, ASYNC_TICK_INTERVAL);
//     tick_timer->start();
// }

void init(const size_type num_threads_in)
{
#ifndef NDEBUG
    static bool did_init = false;
    assert(! did_init);
    did_init = true;
#endif

    for(size_type i = 0; i < num_threads_in; i++) {
        async_threads.emplace_back(async_thread);
    }
}

base_timer::base_timer(const duration_type& initial_in, const duration_type& repeat_in)
: initial(initial_in), repeat(repeat_in), boost_timer(boost_io)
{ }

base_timer::base_timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in)
: initial(initial_in), repeat(repeat_in), boost_timer(boost_io)
{
    watch(handler_in);
}

base_timer::base_timer(const duration_type& initial_in, handler_type handler_in)
: initial(initial_in), repeat(0), boost_timer(boost_io)
{
    watch(handler_in);
}

timer::timer(const duration_type& initial_in, const duration_type& repeat_in)
: base_timer(initial_in, repeat_in)
{ }

timer::timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in)
: base_timer(initial_in, repeat_in, handler_in)
{ }

timer::timer(const duration_type& initial_in, handler_type handler_in)
: base_timer(initial_in, handler_in)
{ }

void base_timer::watch(timer::handler_type handler_in)
{
    lock_type lock(mutex);
    watchers.push_back(handler_in);
}

void base_timer::boost_handler(const boost::system::error_code& error_in)
{
    lock_type lock(mutex);

    if (error_in) {
        if (error_in == boost::asio::error::operation_aborted) {
            return;
        }

        system_fault("there was an error");
    }

    for(auto&& cb : watchers) {
        cb(*this);
    }

    run();

    if (repeat == 0ms) {
        return;
    }

    auto next_time = boost_timer.expires_at() + repeat;
    boost_timer.expires_at(next_time);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);
    boost_timer.async_wait(bound);
}

void base_timer::run()
{ }

void base_timer::start()
{
    lock_type lock(mutex);

    boost_timer.expires_at(std::chrono::system_clock::now() + initial);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);
    boost_timer.async_wait(bound);
}

watchdog::watchdog(const duration_type& timeout_in)
: base_timer(timeout_in, timeout_in)
{ }

void watchdog::start()
{
    base_timer::start();
}

void watchdog::reset()
{
    auto when = std::chrono::system_clock::now() + repeat;
    boost_timer.expires_at(when);
    start();
}

void watchdog::run()
{
    system_fault("watchdog hit timeout");
}

} // namespace async

} // namespace pulsar

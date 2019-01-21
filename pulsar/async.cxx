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

#include "async.h"
#include "logging.h"
#include "system.h"

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
    throw std::runtime_error("Boost io.run() returned");
}

// void init_tick()
// {
//     assert(tick_timer == nullptr);

//     tick_timer = timer::make(0s, ASYNC_TICK_INTERVAL);
//     tick_timer->start();
// }

void init(const size_type num_threads_in)
{
    static bool did_init = false;

    assert(! did_init);

    did_init = true;

    for(size_type i = 0; i < num_threads_in; i++) {
        async_threads.emplace_back(async_thread);
    }
}

base::timer::timer(const duration_type& initial_in, const duration_type& repeat_in)
: initial(initial_in), repeat(repeat_in), boost_timer(boost_io)
{
}

timer::timer(const duration_type& initial_in, const duration_type& repeat_in)
: base::timer(initial_in, repeat_in)
{
}

void base::timer::watch(timer::handler_type handler_in)
{
    lock_type lock(mutex);
    watchers.push_back(handler_in);
}

void base::timer::boost_handler(const boost::system::error_code& error_in)
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

void base::timer::run()
{

}

void base::timer::start()
{
    lock_type lock(mutex);

    boost_timer.expires_at(std::chrono::system_clock::now() + initial);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);
    boost_timer.async_wait(bound);
}

watchdog::watchdog(const duration_type& timeout_in)
: base::timer(timeout_in, timeout_in)
{

}

void watchdog::start()
{
    base::timer::start();
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

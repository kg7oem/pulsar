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
#include <pulsar/debug.h>
#include <pulsar/domain.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

#define DEFAULT_NUM_THREADS 1

namespace pulsar {

namespace async {

using namespace std::chrono_literals;

static boost::asio::io_service boost_io;
static std::vector<thread_type> async_threads;
static std::atomic<bool> is_online_flag = ATOMIC_VAR_INIT(false);

bool is_online()
{
    return is_online_flag.load();
}

// this must be called from outside ASIO
void wait_stopped()
{
    for(auto&& thread : async_threads) {
        thread.join();
    }

    log_debug("engine is stopped");
}

boost::asio::io_service& get_boost_io()
{
    return boost_io;
}

static void async_thread()
{
    log_trace("async thread is giving control to ASIO");
    boost_io.run();
    log_trace("async thread got control back from ASIO");

    if (is_online_flag) system_fault("Boost io.run() returned but async is online");
    log_debug("async processing thread is done running");
}

void init(const size_type num_threads_in)
{
    if (is_online_flag) system_fault("attempt to double init async system");

    auto num_threads = num_threads_in;

    log_trace("initializing async system; specified number of threads: ", num_threads);

    if (num_threads == 0) {
        log_trace("trying to detect number of cores");
        num_threads = std::thread::hardware_concurrency();
    }

    if (num_threads == 0) {
        log_info("unable to detect number of CPU cores; setting num_threads = ", DEFAULT_NUM_THREADS);
        num_threads = DEFAULT_NUM_THREADS;
    }

    log_info("number of threads: ", num_threads);

    for(size_type i = 0; i < num_threads; i++) {
        async_threads.emplace_back(async_thread);
        thread::set_realtime_priority(async_threads.back(), thread::rt_priorty::lowest);
    }

    is_online_flag.store(true);
}

// this has to be safe to call from inside
// the ASIO threads
void stop()
{
    log_debug("shutting down async system");

    if (! is_online_flag) system_fault("attempt to stop the async system but it is not running");

    for(auto&& domain : get_domains()) {
        log_debug("stopping domain ", domain->name);
        domain->shutdown();
    }

    log_trace("stopping ASIO");
    is_online_flag.store(false);
    boost_io.stop();
    log_debug("done telling ASIO to stop");
}

base_timer::base_timer(const duration_type& initial_in, const duration_type& repeat_in)
: initial(initial_in), repeat(repeat_in), boost_timer(boost_io)
{ }

base_timer::~base_timer()
{
    if (running_flag && is_online_flag) {
        system_fault("can not destroy a running timer unless the async system is not online");
    }
}

void base_timer::boost_handler(const boost::system::error_code& error_in)
{
    lock_type lock(mutex);

    if (error_in == boost::asio::error::operation_aborted) {
        assert(running_flag);

        running_flag = false;
        running_condition.notify_all();

        return;
    } else if (error_in) {
        system_fault("boost reported an error to the timer handler");
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

void base_timer::start()
{
    lock_type lock(mutex);

    boost_timer.expires_at(std::chrono::system_clock::now() + initial);
    auto bound = boost::bind(&timer::boost_handler, this, boost::asio::placeholders::error);

    running_flag = true;
    boost_timer.async_wait(bound);

    running_condition.notify_all();
}

void base_timer::reset()
{
    lock_type lock(mutex);

    auto when = std::chrono::system_clock::now() + repeat;
    boost_timer.expires_at(when);
    base_timer::start();
}

void base_timer::stop()
{
    lock_type lock(mutex);
    boost_timer.cancel();
    running_condition.wait(lock, [this]{ return running_flag == false; });
}

timer::timer(const duration_type& initial_in, const duration_type& repeat_in)
: base_timer(initial_in, repeat_in)
{ }

timer::timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in)
: base_timer(initial_in, repeat_in)
{
    watchers.push_back(handler_in);
}

timer::timer(const duration_type& initial_in, handler_type handler_in)
: base_timer(initial_in)
{
    watchers.push_back(handler_in);
}

void timer::watch(timer::handler_type handler_in)
{
    lock_type lock(mutex);
    watchers.push_back(handler_in);
}

void timer::run()
{
    for(auto&& cb : watchers) {
        cb(*this);
    }
}

watchdog::watchdog(const duration_type& timeout_in)
: base_timer(timeout_in, timeout_in)
{ }

watchdog::watchdog(const duration_type& timeout_in, const string_type& message_in)
: base_timer(timeout_in, timeout_in), message(message_in)
{ }

void watchdog::run()
{
    system_fault(message);
}

} // namespace async

} // namespace pulsar

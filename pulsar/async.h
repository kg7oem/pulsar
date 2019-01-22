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

#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <chrono>
#include <functional>
#include <memory>

#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace async {

using boost_timer_type = boost::asio::system_timer;

void init(const size_type num_threads_in = 1);
// boost::asio::io_service& get_global_io();

struct base_timer {
    using handler_type = std::function<void (base_timer&)>;

    protected:
    mutex_type mutex;
    duration_type initial;
    duration_type repeat;
    boost_timer_type boost_timer;
    std::vector<handler_type> watchers;
    virtual void boost_handler(const boost::system::error_code& error_in);
    virtual void run();

    public:
    base_timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    base_timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in);
    base_timer(const duration_type& initial_in, handler_type handler_in);
    virtual ~base_timer();
    virtual void start();
    virtual void reset();
    virtual void stop();
    virtual void watch(handler_type handler_in);
};

struct timer : public base_timer, public std::enable_shared_from_this<timer> {
    timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    timer(const duration_type& initial_in, const duration_type& repeat_in, handler_type handler_in);
    timer(const duration_type& initial_in, handler_type handler_in);
    template <typename... Args>
    static std::shared_ptr<timer> make(Args&&... args)
    {
        return std::make_shared<timer>(args...);
    }
};

struct watchdog : public base_timer, public std::enable_shared_from_this<watchdog> {
    watchdog(const duration_type& timeout_in);
    template <typename... Args>
    static std::shared_ptr<watchdog> make(Args&&... args)
    {
        return std::make_shared<watchdog>(args...);
    }
    virtual void run() override;
};

} // namespace async

} // namespace pulsar

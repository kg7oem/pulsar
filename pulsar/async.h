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

#include "system.h"
#include "thread.h"

namespace pulsar {

namespace async {

using boost_timer_type = boost::asio::system_timer;
using duration_type = std::chrono::milliseconds;

void init(const size_type num_threads_in = 1);
// boost::asio::io_service& get_global_io();

namespace base {

struct timer {
    using handler_type = std::function<void (timer&)>;

    protected:
    mutex_type mutex;
    duration_type initial;
    duration_type repeat;
    boost_timer_type boost_timer;
    std::vector<handler_type> watchers;
    void boost_handler(const boost::system::error_code& error_in);
    virtual void run();

    public:
    timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    void start();
    void watch(handler_type handler_in);
};

} // namespace base

struct timer : public base::timer, public std::enable_shared_from_this<timer> {
    timer(const duration_type& initial_in, const duration_type& repeat_in = duration_type(0));
    template <typename... Args>
    static std::shared_ptr<timer> make(Args&&... args)
    {
        return std::make_shared<timer>(args...);
    }
};

struct watchdog : protected base::timer, public std::enable_shared_from_this<watchdog> {
    watchdog(const duration_type& timeout_in);
    template <typename... Args>
    static std::shared_ptr<watchdog> make(Args&&... args)
    {
        return std::make_shared<watchdog>(args...);
    }
    void start();
    void reset();
    virtual void run() override;
};

} // namespace async

} // namespace pulsar

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

#include <boost/version.hpp>
#include <stdexcept>
#include <string>

#include <pulsar/async.h>
#include <pulsar/jackaudio.h>
#include <pulsar/ladspa.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

#define ALIVE_TICK_INTERVAL 100ms

namespace pulsar {

namespace system {

using namespace std::chrono_literals;

static std::shared_ptr<async::timer> alive_timer;

void bootstrap()
{
    pulsar::node::init();
    pulsar::jackaudio::init();
    pulsar::ladspa::init();

    // the timer must exist before async init happens
    alive_timer = async::timer::make(0s, ALIVE_TICK_INTERVAL);
    alive_timer->start();

    pulsar::async::init();
}

void register_alive_handler(alive_handler_type cb_in, void * arg_in)
{
    auto wrapper = [cb_in, arg_in](async::base::timer&) {
        cb_in(arg_in);
    };

    alive_timer->watch(wrapper);
}

string_type make_boost_version();

const string_type boost_version = make_boost_version();

const string_type& get_boost_version()
{
    return boost_version;
}

string_type make_boost_version()
{
    string_type buf("v");
    string_type version(BOOST_LIB_VERSION);

    auto pos = version.find("_");

    if (pos == string_type::npos) {
        system_fault("expected to find _ in " + version);
    }

    version.replace(pos, 1, ".");

    pos = version.find("_");

    if (pos == string_type::npos) {
        version += ".0";
    } else {
        version.replace(pos, 1, ".");
    }

    buf += version;

    return buf;
}

[[noreturn]] void fault(const char* file_in, int line_in, const char* function_in, const string_type& message_in)
{
    logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::fatal, function_in, file_in, line_in, message_in);
    abort();
}

} // namespace system

} // namespace pulsar

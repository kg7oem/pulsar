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
#include <iostream>
#include <logjam/logjam.h>
#include <memory>
#include <stdexcept>
#include <string>

#include <pulsar/async.h>
#include <pulsar/ladspa.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

#ifdef CONFIG_ENABLE_DBUS
#include <pulsar/dbus.h>
#endif

#ifdef CONFIG_ENABLE_JACKAUDIO
#include <pulsar/jackaudio.h>
#endif

#ifdef CONFIG_ENABLE_PORTAUDIO
#include <pulsar/portaudio.h>
#endif

#define ALIVE_TICK_INTERVAL 100ms

namespace pulsar {

namespace system {

using namespace std::chrono_literals;

static std::shared_ptr<async::timer> alive_timer;
static std::shared_ptr<logjam::logmemory> memory_logger;

[[noreturn]] void fault(const char* file_in, int line_in, const char* function_in, const string_type& message_in)
{
    std::cerr << "Pulsar faulted: " << message_in << " at " << file_in << ":" << line_in << std::endl;
    logjam::send_vargs_logevent(PULSAR_LOG_NAME, logjam::loglevel::fatal, function_in, file_in, line_in, message_in);

    if (memory_logger != nullptr) {
        std::cerr << "Contents of in memory log:" << std::endl;

        for(auto&& log_message : memory_logger->format_event_history()) {
            std::cerr << log_message;
        }
    }

    abort();
}

void bootstrap(const size_type num_threads_in)
{
#ifdef CONFIG_ENABLE_DBUS
    pulsar::dbus::init();
#endif

    pulsar::node::init();

#ifdef CONFIG_ENABLE_LADSPA
    pulsar::ladspa::init();
#endif

#ifdef CONFIG_ENABLE_JACKAUDIO
    pulsar::jackaudio::init();
#endif

#ifdef CONFIG_ENABLE_PORTAUDIO
    pulsar::portaudio::init();
#endif

    // the timer must exist before async init happens
    alive_timer = async::timer::make(0s, ALIVE_TICK_INTERVAL);
    alive_timer->start();

    pulsar::async::init(num_threads_in);
}

void shutdown()
{
    async::stop();
}

void wait_stopped()
{
    async::wait_stopped();
}

void register_alive_handler(alive_handler_type cb_in, void * arg_in)
{
    auto wrapper = [cb_in, arg_in](async::base_timer&) {
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

void enable_memory_logging(const duration_type& max_age_in, const string_type& level_name_in)
{
    if (memory_logger != nullptr) {
        system_fault("attempt to enable the memory logger twice");
    }

    memory_logger = std::make_shared<logjam::logmemory>(logjam::level_from_name(level_name_in));
    memory_logger->set_max_age(max_age_in);
    logjam::logengine::get_engine()->add_destination(memory_logger);
}

} // namespace system

} // namespace pulsar

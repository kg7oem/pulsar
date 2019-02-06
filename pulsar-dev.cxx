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

#include <csignal>
#include <iostream>

#include <pulsar/async.h>
#include <pulsar/daemon.h>
#include <pulsar/config.h>
#include <pulsar/debug.h>
#include <pulsar/library.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

using namespace std;
using namespace std::chrono_literals;

#define INFO_DELAY 50ms
// Give valgrind lots of time
#define ALARM_TIMEOUT 1

#define DEFAULT_CONSOLE_LOG_LEVEL "info"
// #define DEFAULT_MEMORY_LOG_LEVEL "debug"
#define DEFAULT_MEMORY_LOG_AGE 3s

static std::shared_ptr<logjam::logmemory> memory_logger;
static std::list<std::shared_ptr<pulsar::daemon::base>> active_daemons;

static void init_logging(std::shared_ptr<pulsar::config::file> config_in)
{
    auto logging = logjam::logengine::get_engine();
    auto engine = config_in->get_engine();
    auto engine_logs = engine["logs"];
    std::string log_level_name;

    if (engine_logs) {
        auto memory_level_node = engine_logs["memory_level"];

        if (memory_level_node) {
            auto memory_age_node = engine_logs["memory_age"];
            auto memory_level_name = memory_level_node.as<std::string>();
            pulsar::duration_type duration;

            if (memory_age_node) {
                auto memory_age_num = memory_age_node.as<pulsar::size_type>();
                duration = std::chrono::seconds(memory_age_num);
            } else {
                duration = DEFAULT_MEMORY_LOG_AGE;
            }

            pulsar::system::enable_memory_logging(duration, memory_level_name);
        }

        auto console_level_node = engine_logs["console_level"];

        if (console_level_node) {
            log_level_name = console_level_node.as<std::string>();
        }
    }

    if (log_level_name == "") {
        log_level_name = DEFAULT_CONSOLE_LOG_LEVEL;
    }

    static auto console = make_shared<logjam::logconsole>(logjam::level_from_name(log_level_name));

    auto log_sources_node = engine_logs["console_sources"];

    if (log_sources_node) {
        if (log_sources_node.IsScalar()) {
            console->add_source_filter(log_sources_node.as<std::string>());
        } else if (log_sources_node.IsSequence()) {
            for(unsigned long i = 0; i < log_sources_node.size(); i++) {
                console->add_source_filter(log_sources_node[i].as<std::string>());
            }
        } else {
            system_fault("invalid node type for log sources section");
        }
    }

    logging->add_destination(console);
    logging->start();
}

static void init_debug(std::shared_ptr<pulsar::config::file> config_in)
{
    auto engine_section = config_in->get_engine();
    auto debug_section = engine_section["debug"];

    if (! debug_section) return;
    if (! debug_section.IsMap()) system_fault("debug section of config file was not a map");
}

static void alarm_handler(const int signum_in)
{
    assert(signum_in == SIGALRM);
    system_fault("caught alarm");
}

UNUSED static void init_pulsar(const pulsar::size_type num_threads_in)
{
    std::signal(SIGALRM, alarm_handler);
    alarm(ALARM_TIMEOUT);

    pulsar::system::bootstrap(num_threads_in);

    pulsar::system::register_alive_handler([&] (void *) {
        alarm(ALARM_TIMEOUT);
    });
}

static void init_signals()
{
    auto&& io = pulsar::async::get_boost_io();
    static boost::asio::signal_set quit_signals(io, SIGINT, SIGTERM);
    // static boost::asio::signal_set fault_signals(io, SIGSEGV, SIGABRT, SIGBUS);

    quit_signals.async_wait([](const boost::system::error_code& error_in, const int) {
        if (error_in) {
            system_fault("got an error from ASIO in the quit signal handler");
        }

        pulsar::system::shutdown();
    });

    // fault_signals.async_wait([](const boost::system::error_code& error_in, const int signum_in) {
    //     if (error_in) {
    //         system_fault("got an error from ASIO in the fault signal handler");
    //     }

    //     system_fault("fatal signal received: ", signum_in);
    // });
}

UNUSED static void init(std::shared_ptr<pulsar::config::file> config_in)
{
    init_logging(config_in);
    init_debug(config_in);

    auto engine_node = config_in->get_engine()["threads"];
    pulsar::size_type num_threads = 0;

    if (engine_node) {
        num_threads = engine_node.as<pulsar::size_type>();
    }

    init_pulsar(num_threads);
    init_signals();
}

UNUSED void log_properties(pulsar::node::base * node_in)
{

    llog_debug({
        auto& node_name = node_in->get_property("node:name").value->get_string();
        return pulsar::util::to_string("Properties for node: ", node_name);
    });

    for(auto&& i : node_in->get_properties()) {
        if (i.first == "node:name") {
            continue;
        }

        llog_debug({
            auto property = i.second;
            return pulsar::util::to_string("property: ", property.name, "; value = '", property.value->get(), "'");
        });
    }
}

std::string get_compressor_state(pulsar::node::base * node_in)
{
    std::string buf;

    buf += node_in->name + " ";
    buf += "reduction: " + node_in->peek("state:Gain Reduction") + " dB; ";
    buf += "output level: " + node_in->peek("state:Output Level") + " dB";

    return buf;
}

UNUSED static void process_audio(std::shared_ptr<pulsar::config::file> config_in)
{
    log_info("Configuring audio processing");

    auto domain_info = config_in->get_domain();
    auto domain = pulsar::config::make_domain(domain_info);
    auto node_map = pulsar::config::make_nodes(domain_info, domain);

    auto daemons_section = config_in->get_daemons();
    if (daemons_section) {
        for(auto&& i : daemons_section) {
            auto daemon_name = i.first.as<pulsar::string_type>();
            auto daemon_contents = i.second;
            auto daemon_class = daemon_contents["class"].as<pulsar::string_type>();

            auto new_daemon = pulsar::library::make_daemon(daemon_class, daemon_name);
            new_daemon->init(daemon_contents["config"]);
            new_daemon->start();
            active_daemons.push_back(new_daemon);
        }
    }

    log_info("audio processing is being started");
    domain->activate();

    std::vector<pulsar::node::base *> compressor_nodes;
    compressor_nodes.push_back(node_map["comp_right"]);
    compressor_nodes.push_back(node_map["comp_left"]);
    compressor_nodes.push_back(node_map["tail_eater"]);
}

int main(int argc_in, const char ** argv_in)
{
    if (argc_in != 2) {
        system_fault("must specify a configuration file on the command line");
    }

    auto config = pulsar::config::file::make(argv_in[1]);

    init(config);

    log_info("pulsar-dev initialized");
    log_info("Using Boost ", pulsar::system::get_boost_version());

    process_audio(config);
    pulsar::system::wait_stopped();

    log_info("done processing audio");
    return 0;
}

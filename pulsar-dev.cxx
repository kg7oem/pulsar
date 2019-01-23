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

#include <iostream>

#include <pulsar/async.h>
#include <pulsar/config.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

using namespace std;
using namespace std::chrono_literals;

#define INFO_DELAY 50ms
// Give valgrind lots of time
#define ALARM_TIMEOUT 5

#define DEFAULT_CONSOLE_LOG_LEVEL "info"
// #define DEFAULT_MEMORY_LOG_LEVEL "debug"
#define DEFAULT_MEMORY_LOG_AGE 5s

static std::shared_ptr<logjam::logmemory> memory_logger;

static void init_logging(std::shared_ptr<pulsar::config::file> config_in)
{
    auto logging = logjam::logengine::get_engine();
    auto engine = config_in->get_engine();
    auto engine_logs = engine["logs"];
    string_type log_level_name;

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

    auto console = make_shared<logjam::logconsole>(logjam::level_from_name(log_level_name));
    logging->add_destination(console);

    logging->start();
}

UNUSED static void init_pulsar()
{
    pulsar::system::bootstrap();

    pulsar::system::register_alive_handler([&] (void *) {
        alarm(ALARM_TIMEOUT);
    });

    alarm(ALARM_TIMEOUT);
}

UNUSED static void init(std::shared_ptr<pulsar::config::file> config_in)
{
    init_logging(config_in);
    init_pulsar();
}

UNUSED void log_properties(pulsar::node::base * node_in)
{

    llog_debug({
        auto& node_name = node_in->get_property("node:name").get_string();
        return pulsar::util::to_string("Properties for node: ", node_name);
    });

    for(auto&& i : node_in->get_properties()) {
        if (i.first == "node:name") {
            continue;
        }

        llog_debug({
            auto property = i.second;
            return pulsar::util::to_string("property: ", property->name, "; value = '", property->get(), "'");
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
    log_info("Will start processing audio");

    auto domain_info = config_in->get_domain();
    auto domain = pulsar::config::make_domain(domain_info);
    auto domain_num_threads = domain_info->get_config()["threads"].as<pulsar::size_type>();
    auto node_map = pulsar::config::make_nodes(domain_info, domain);

    log_debug("audio processing is being started");
    domain->activate(domain_num_threads);

    for(UNUSED auto&& node : node_map) {
        log_properties(node.second);
    }

    std::vector<pulsar::node::base *> compressor_nodes;
    compressor_nodes.push_back(node_map["comp_right"]);
    compressor_nodes.push_back(node_map["comp_left"]);
    compressor_nodes.push_back(node_map["tail_eater"]);

    auto info_timer = pulsar::async::timer::make(
        INFO_DELAY, INFO_DELAY, [compressor_nodes](pulsar::async::base_timer&) {
            for(UNUSED auto&& compressor : compressor_nodes) {
            log_verbose(get_compressor_state(compressor));
        }
    });

    info_timer->start();

    while(1) {
        std::this_thread::sleep_for(1s);
    }
}

int main(void)
{
    auto config = pulsar::config::file::make("dev-config.yaml");

    init(config);

    log_info("pulsar-dev initialized");
    log_info("Using Boost ", pulsar::system::get_boost_version());

    process_audio(config);

    return 0;
}

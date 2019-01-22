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

#include <pulsar/async.h>
#include <pulsar/config.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

using namespace std;
using namespace std::chrono_literals;

#define LOG_LEVEL info
#define INFO_DELAY 50ms
// Give valgrind lots of time
#define ALARM_TIMEOUT 5

static std::shared_ptr<logjam::logmemory> memory_logger;

static void init_logging()
{
    auto logging = logjam::logengine::get_engine();

    auto console = make_shared<logjam::logconsole>(logjam::loglevel::LOG_LEVEL);
    logging->add_destination(console);

    memory_logger = make_shared<logjam::logmemory>(logjam::loglevel::trace);
    logging->add_destination(memory_logger);

    logging->start();
}

UNUSED static void init_pulsar()
{
    pulsar::system::bootstrap();

    pulsar::system::register_alive_handler([&] (void *) {
        assert(memory_logger != nullptr);

        memory_logger->cleanup();

        alarm(ALARM_TIMEOUT);
    });

    alarm(ALARM_TIMEOUT);
}

UNUSED static void init()
{
    init_logging();
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

UNUSED static void process_audio()
{
    log_info("Will start processing audio");

    auto config = pulsar::config::file::make("dev-config.yaml");
    auto domain_info = config->get_domain();
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
    init();

    log_info("pulsar-dev initialized");
    log_info("Using Boost ", pulsar::system::get_boost_version());

    process_audio();

    return 0;
}

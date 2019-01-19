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

#include <memory>
#include <iostream>
#include <unistd.h>

#include "pulsar/async.h"
#include "pulsar/config.h"
#include "pulsar/domain.h"
#include "pulsar/jackaudio.h"
#include "pulsar/ladspa.h"
#include "pulsar/library.h"
#include "pulsar/logging.h"
#include "pulsar/property.h"
#include "pulsar/system.h"

using namespace std;
using namespace std::chrono_literals;

#define ALARM_TIMEOUT 1

static void init_logging()
{
    auto logging = logjam::logengine::get_engine();
    auto console = make_shared<logjam::logconsole>(logjam::loglevel::debug);

    logging->add_destination(console);
    logging->start();
}

UNUSED static void init_pulsar()
{
    pulsar::async::init();
    pulsar::jackaudio::init();
    pulsar::ladspa::init();

    pulsar::async::register_tick_handler([] (void *) {
        alarm(ALARM_TIMEOUT);
    });

    alarm(ALARM_TIMEOUT);
}

UNUSED static void init()
{
    init_logging();
}

pulsar::node::base::node * setup_ladspa(pulsar::node::base::node * node_in)
{
    node_in->get_property("plugin:filename").set("/usr/lib/ladspa/amp.so");
    node_in->get_property("plugin:id").set(1048);

    node_in->init();
    return node_in;
}

void log_properties(pulsar::node::base::node * node_in)
{
    auto& node_name = node_in->get_property("node:name").get_string();

    log_debug("Properties for node: ", node_name);

    for(auto&& i : node_in->get_properties()) {
        if (i.first == "node:name") {
            continue;
        }

        auto property = i.second;
        log_debug("property: ", property->name, "; value = '", property->get(), "'");
    }
}

UNUSED static void process_audio()
{

    log_info("Will start processing audio");


    auto config = pulsar::config::file::make("dev-config.yaml");
    auto domain_info = config->get_domain();
    auto domain = pulsar::config::make_domain(domain_info);
    auto domain_num_threads = domain_info->get_config()["threads"].as<pulsar::size_type>();
    auto node_list = pulsar::config::make_nodes(domain_info, domain);

    log_debug("audio processing is being started");
    domain->activate(domain_num_threads);

    for(auto&& node : node_list) {
        log_properties(node);
    }

    while(1) {
        std::this_thread::sleep_for(1s);
    }
}

int main(void)
{
    init();
    init_pulsar();

    log_info("pulsar-dev initialized");
    log_info("Using Boost ", pulsar::system::get_boost_version());

    process_audio();

    return 0;
}

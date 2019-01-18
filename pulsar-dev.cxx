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
#include "pulsar/configfile.h"
#include "pulsar/domain.h"
#include "pulsar/jackaudio.h"
#include "pulsar/ladspa.h"
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

static void init_pulsar()
{
    pulsar::async::init();

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
    init_pulsar();

    auto config = pulsar::configfile::file::make("dev-config.yaml");
    auto domain_name = std::string("main");
    auto domain_config = config->get_domain(domain_name)["config"];
    auto domain_sample_rate = domain_config["sample_rate"].as<pulsar::size_type>();
    auto domain_buffer_size = domain_config["buffer_size"].as<pulsar::size_type>();
    auto domain_num_threads = domain_config["threads"].as<pulsar::size_type>();

    log_info("Will start processing audio");

    auto domain = pulsar::domain::make(domain_name, domain_sample_rate, domain_buffer_size);
    auto gain_left = setup_ladspa(domain->make_node<pulsar::ladspa::node>("left"));
    auto gain_right = setup_ladspa(domain->make_node<pulsar::ladspa::node>("right"));
    auto jack = domain->make_node<pulsar::jackaudio::node>("pulsar");

    gain_left->get_property("config:Gain").set(1);
    gain_right->get_property("config:Gain").set(1);

    jack->audio.add_output("in_left")->connect(gain_left->audio.get_input("Input"));
    jack->audio.add_output("in_right")->connect(gain_right->audio.get_input("Input"));

    jack->audio.add_input("out_left")->connect(gain_left->audio.get_output("Output"));
    jack->audio.add_input("out_right")->connect(gain_right->audio.get_output("Output"));

    domain->activate(domain_num_threads);

    log_properties(jack);
    log_properties(gain_left);
    log_properties(gain_right);

    log_debug("audio processing is running");

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

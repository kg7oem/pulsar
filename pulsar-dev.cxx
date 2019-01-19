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

std::shared_ptr<pulsar::domain> make_domain(std::shared_ptr<pulsar::config::domain> domain_info_in)
{
    auto domain_config = domain_info_in->get_config();

    assert(domain_config["sample_rate"]);
    assert(domain_config["buffer_size"]);

    auto domain_sample_rate = domain_config["sample_rate"].as<pulsar::size_type>();
    auto domain_buffer_size = domain_config["buffer_size"].as<pulsar::size_type>();

    auto domain = pulsar::domain::make(domain_info_in->name, domain_sample_rate, domain_buffer_size);
    return domain;
}

// from https://stackoverflow.com/a/236803
template<typename Out>
void split(const std::string &string_in, const char delim_in, Out result) {
    std::stringstream ss(string_in);
    std::string item;
    while (std::getline(ss, item, delim_in)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string& string_in, const char delim_in)
{
    std::vector<std::string> parts;
    split(string_in, delim_in, std::back_inserter(parts));
    return parts;
}

std::vector<pulsar::node::base::node *> make_nodes(std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in) {
    auto node_map = std::map<std::string, pulsar::node::base::node *>();

    for (auto&& node_yaml : config_in->get_nodes()) {
        auto node_name = node_yaml["name"].as<std::string>();
        auto class_name = node_yaml["class"].as<std::string>();
        auto node_config = node_yaml["config"];
        auto node_plugin = node_yaml["plugin"];
        auto node_inputs = node_yaml["inputs"];
        auto node_outputs = node_yaml["outputs"];
        auto new_node = pulsar::library::make_node(class_name, node_name, domain_in);

        if (node_plugin) {
            for(auto&& i : node_plugin) {
                auto config_name = i.first.as<std::string>();
                auto config_node = i.second;
                auto property_name = std::string("plugin:") + config_name;
                new_node->get_property(property_name).set(config_node);
            }
        }

        new_node->init();

        if (node_config) {
            for(auto&& i : node_config) {
                auto config_name = i.first.as<std::string>();
                auto config_node = i.second;
                auto property_name = std::string("config:") + config_name;
                new_node->get_property(property_name).set(config_node);
            }
        }

        if (node_inputs) {
            for(auto&& i : node_inputs) {
                auto input_name = i.as<std::string>();
                new_node->audio.add_input(input_name);
            }
        }

        if (node_outputs) {
            for(auto&& i : node_outputs) {
                auto output_name = i.as<std::string>();
                new_node->audio.add_output(output_name);
            }
        }

        auto found = node_map.find(node_name);
        if (found != node_map.end()) {
            system_fault("duplicate node name: ", node_name);
        }
        node_map[node_name] = new_node;
    }

    for (auto&& node_yaml : config_in->get_nodes()) {
        auto node_name = node_yaml["name"].as<std::string>();
        auto connections = node_yaml["connect"];

        if (! connections) {
            continue;
        }

        for(auto&& i : connections) {
            auto source_channel = i.first.as<std::string>();
            auto target_string = i.second.as<std::string>();
            auto target_split = split(target_string, ':');

            if (target_split.size() != 2) {
                system_fault("malformed connect target: ", target_string);
            }

            log_debug("Connect ", node_name, ":", source_channel, " -> ", target_string);

            auto sink_node_name = target_split[0];
            auto sink_channel_name = target_split[1];

            if (node_map.find(node_name) == node_map.end()) {
                system_fault("could not find source node named ", node_name);
            }

            if (node_map.find(sink_node_name) == node_map.end()) {
                system_fault("could not find sink node named ", sink_node_name);
            }

            UNUSED auto source_node = node_map[node_name];
            UNUSED auto sink_node = node_map[sink_node_name];
            UNUSED auto sink_channel = sink_node->audio.get_input(sink_channel_name);

            source_node->audio.get_output(source_channel)->connect(sink_channel);
        }
    }

    auto node_list = std::vector<pulsar::node::base::node *>();

    for(auto&& i : node_map) {
        node_list.push_back(i.second);
    }

    return node_list;
}

UNUSED static void process_audio()
{

    log_info("Will start processing audio");


    auto config = pulsar::config::file::make("dev-config.yaml");
    auto domain_info = config->get_domain();
    auto domain = make_domain(domain_info);
    auto domain_num_threads = domain_info->get_config()["threads"].as<pulsar::size_type>();
    auto node_list = make_nodes(domain_info, domain);

    for(auto&& node : node_list) {
        log_properties(node);
    }

    log_debug("audio processing is running");
    domain->activate(domain_num_threads);

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

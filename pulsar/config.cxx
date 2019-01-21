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

#include <cassert>

#include "config.h"
#include "domain.h"
#include "library.h"
#include "logging.h"
#include "system.h"
#include "util.h"

namespace pulsar {

namespace config {

static pulsar::node::base::node * make_node(const YAML::Node& node_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);
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

static void apply_node_template(YAML::Node& dest_in, const YAML::Node& src_in)
{
    if (dest_in.Type() != src_in.Type()) {
        system_fault("YAML node type difference during template import");
    }

    if (src_in.IsMap()) {
        for(auto&& i : src_in) {
            auto key_name = i.first.as<std::string>();

            if (dest_in[key_name]) {
                if (src_in[key_name].IsSequence() || src_in[key_name].IsMap()) {
                    // FIXME is that how this is supposed to work?
                    auto new_node = dest_in[key_name];
                    apply_node_template(new_node, src_in[key_name]);
                }
            } else {
                dest_in[key_name] = i.second;
            }
        }
    } else if (src_in.IsSequence()) {
        // FIXME does it make sense to try to merge a sequence?
        // Does not seem so
        // auto size = src_in.size();
        // for(size_type i = 0; i < size; i++) {
        //     if (dest_in[i]) {
        //         if (src_in[i].IsSequence() || src_in[i].IsMap()) {
        //             // FIXME is that how this is supposed to work?
        //             auto new_node = dest_in[i];
        //             apply_node_template(new_node, src_in[i]);
        //         }
        //     } else {
        //         dest_in[i] = src_in[i];
        //     }
        // }
    }
}

static void connect_nodes(std::map<std::string, node::base::node *>& node_map_in, const YAML::Node& node_yaml_in)
{
    auto node_yaml = node_yaml_in;
    auto node_name = node_yaml["name"].as<std::string>();
    auto source_node = node_map_in[node_name];
    auto connections = node_yaml["connect"];
    auto forwards = node_yaml["forward"];

    if (connections) {
        // if the number of inputs and outputs is the same
        // connect them together in order
        if (connections.IsScalar()) {
            auto output_names = source_node->audio.get_output_names();
            auto sink_node_name = connections.as<std::string>();

            if (node_map_in.find(sink_node_name) == node_map_in.end()) {
                system_fault("could not find a node named ", sink_node_name);
            }

            auto sink_node = node_map_in[sink_node_name];
            auto input_names = sink_node->audio.get_input_names();

            auto num_outputs = output_names.size();
            if (input_names.size() != num_outputs) {
                system_fault("number of inputs and outputs was not the same");
            }

            for(size_type i = 0; i < num_outputs; i++) {
                sink_node->audio.get_input(input_names[i])->connect(source_node, output_names[i]);
                // auto sink_channel = sink_node->audio.get_input(input_names[i]);
                // source_node->audio.get_output(output_names[i])->link_to(sink_channel);
            }
        } else {
            for(auto&& i : connections) {
                auto source_channel = i.first.as<std::string>();
                auto target_node = i.second;

                if (target_node.IsSequence()) {
                    system_fault("can't handle a sequence yet");
                }

                auto target_string = i.second.as<std::string>();
                auto target_split = util::string::split(target_string, ':');
                auto split_size = target_split.size();
                std::string sink_node_name, sink_channel_name;
                node::base::node * sink_node;

                if (split_size == 2) {
                    sink_node_name = target_split[0];
                    auto found = node_map_in.find(sink_node_name);
                    if (found == node_map_in.end()) {
                        system_fault("could not find a node named ", sink_node_name);
                    }
                    sink_node = found->second;
                    sink_channel_name = target_split[1];
                } else if (split_size == 1) {
                    sink_node_name = target_string;
                    auto found = node_map_in.find(sink_node_name);
                    if (found == node_map_in.end()) {
                        system_fault("could not find a node named ", sink_node_name);
                    }
                    sink_node = found->second;
                    auto sink_node_inputs = sink_node->audio.get_input_names();

                    if (sink_node_inputs.size() != 1) {
                        system_fault("no input channel was specified and target node has more than 1 input");
                    }

                    sink_channel_name = sink_node_inputs[0];
                } else {
                    system_fault("invalid connection string specified: ", target_string);
                }

                source_node->audio.get_output(source_channel)->connect(sink_node, sink_channel_name);
            }
        }
    }

    if (forwards) {
        assert(forwards.IsMap());

        for(auto&& i : forwards) {
            auto from_port_string = i.first.as<std::string>();
            auto from_port = source_node->audio.get_output(from_port_string);
            auto target_string = i.second.as<std::string>();
            auto to_parts = util::string::split(target_string, ':');
            auto target_node = node_map_in[to_parts[0]];
            auto target_port = target_node->audio.get_output(to_parts[1]);
            from_port->forward_to(target_port);
        }
    }
}

static pulsar::node::base::node * make_class_node(const YAML::Node& node_yaml_in, std::shared_ptr<pulsar::domain> domain_in)
{
    auto node_yaml = node_yaml_in;
    auto node_name_node = node_yaml["name"];
    auto node_name = node_name_node.as<std::string>();
    auto class_name_node = node_yaml["class"];
    auto class_name = class_name_node.as<std::string>();

    auto node_config_node = node_yaml["config"];
    auto node_plugin_node = node_yaml["plugin"];
    auto node_inputs_node = node_yaml["inputs"];
    auto node_outputs_node = node_yaml["outputs"];

    auto new_node = pulsar::library::make_node(class_name, node_name, domain_in);

    if (node_plugin_node) {
        for(auto&& i : node_plugin_node) {
            auto config_name = i.first.as<std::string>();
            auto config_node = i.second;
            auto property_name = std::string("plugin:") + config_name;
            new_node->get_property(property_name).set(config_node);
        }
    }

    new_node->init();

    if (node_config_node) {
        for(auto&& i : node_config_node) {
            auto config_name = i.first.as<std::string>();
            auto config_node = i.second;
            auto property_name = std::string("config:") + config_name;
            new_node->get_property(property_name).set(config_node);
        }
    }

    if (node_inputs_node) {
        for(auto&& i : node_inputs_node) {
            auto input_name = i.as<std::string>();
            new_node->audio.add_input(input_name);
        }
    }

    if (node_outputs_node) {
        for(auto&& i : node_outputs_node) {
            auto output_name = i.as<std::string>();
            new_node->audio.add_output(output_name);
        }
    }

    return new_node;
}

pulsar::node::base::node * make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in)
{
    YAML::Node node_yaml = node_yaml_in;
    std::map<std::string, node::base::node *> chain_nodes;

    auto chain_name = node_yaml_in["chain"].as<std::string>();
    auto node_name = node_yaml_in["name"].as<std::string>();
    auto chain_channels_node = chain_yaml_in["channels"];
    auto chain_outputs_node = chain_yaml_in["outputs"];
    auto chain_inputs_node = chain_yaml_in["inputs"];
    auto forward_node = chain_yaml_in["forward"];
    auto nodes_node = chain_yaml_in["nodes"];
    auto state_node = chain_yaml_in["state"];

    if (node_yaml_in["class"]) {
        system_fault("chain node had a class set on it");
    }

    if (! nodes_node) {
        system_fault("chain did not have a nodes section");
    } else if (! nodes_node.IsSequence()) {
        system_fault("nodes section in chain was not a sequence");
    }

    if (! forward_node) {
        system_fault("chain did not have a forward section");
    } else if (! forward_node.IsMap()) {
        system_fault("chain forward section was not a map");
    }

    if (! chain_channels_node && ! chain_outputs_node && ! chain_inputs_node) {
        system_fault("chain did not have any outputs or inputs");
    }

    if (chain_channels_node && (chain_outputs_node || chain_inputs_node)) {
        system_fault("chain can have a channels section or input / output sections but not both");
    }

    node_yaml["class"] = "pulsar::node::chain";

    auto chain_root_node = make_class_node(node_yaml_in, domain_in);

    if (chain_outputs_node) {
        if (! chain_outputs_node.IsSequence()) {
            system_fault("outputs section was not a sequence");
        }

        for(size_type i = 0; i < chain_outputs_node.size(); i++) {
            auto output_name = chain_outputs_node[i].as<std::string>();
            chain_root_node->audio.add_output(output_name);
        }
    }

    if (chain_inputs_node) {
        if (! chain_inputs_node.IsSequence()) {
            system_fault("inputs section was not a sequence");
        }

        for(size_type i = 0; i < chain_inputs_node.size(); i++) {
            auto input_name = chain_inputs_node[i].as<std::string>();
            chain_root_node->audio.add_input(input_name);
        }
    }

    if (chain_channels_node) {
        if (! chain_channels_node.IsScalar()) {
            system_fault("channels section was not a scalar");
        }

        auto num_channels = chain_channels_node.as<size_type>();
        // channel counting starts at 1
        for(size_type i = 1; i < num_channels + 1; i++) {
            auto channel_number = std::to_string(i);
            auto input_name = "in_" + channel_number;
            auto output_name = "out_" + channel_number;

            chain_root_node->audio.add_input(input_name);
            chain_root_node->audio.add_output(output_name);
        }
    }

    // store the name of the chain so it can be found and connected
    // to from inside the chain
    chain_nodes[chain_name] = chain_root_node;

    for(size_type i = 0; i < nodes_node.size(); i++) {
        auto new_node = make_node(nodes_node[i], config_in, domain_in);

        if (chain_nodes.find(new_node->name) != chain_nodes.end()) {
            system_fault("duplicate node name in chain: ", new_node->name);
        }

        chain_nodes[new_node->name] = new_node;
    }

    // FIXME this needs to be made common with other places connections
    // are formed
    for(auto&& i : forward_node) {
        UNUSED auto output_name = i.first.as<std::string>();
        auto target_node = i.second;

        if (target_node.IsSequence()) {
            for(size_type i = 0; i < target_node.size(); i++) {
                auto target_string = target_node[i].as<std::string>();
                auto target_parts = util::string::split(target_string, ':');
                auto target_node = chain_nodes[target_parts[0]];
                auto target_input = target_node->audio.get_input(target_parts[1]);
                chain_root_node->audio.get_input(output_name)->forward_to(target_input);
            }
        } else {
            system_fault("can only handle a sequence right now");
        }
    }

    if (state_node) {
        if (! state_node.IsSequence()) {
            system_fault("state section was not a sequence");
        }

        for(size_type i = 0; i < state_node.size(); i++) {
            auto target_string = state_node[i].as<std::string>();
            auto target_parts = util::string::split(target_string, ':');
            assert(target_parts.size() == 2);
            auto target_node_name = target_parts[0];
            auto target_property_name = target_parts[1];
            auto target_node = chain_nodes[target_node_name];

            if (target_property_name != "*") {
                target_property_name = "state:" + target_property_name;
                auto&& property = target_node->get_property(target_property_name);
                chain_root_node->add_property(target_property_name, &property);
            } else {
                for(auto&& i : target_node->get_properties()) {
                    auto property_name = i.first;
                    auto property_parts = util::string::split(property_name, ':');
                    auto prefix = property_parts[0];

                    if (prefix != "state") {
                        continue;
                    }

                    chain_root_node->add_property(property_name, i.second);
                }
            }
        }
    }

    for(size_type i = 0; i < nodes_node.size(); i++) {
        connect_nodes(chain_nodes, nodes_node[i]);
    }

    return chain_root_node;
}

static pulsar::node::base::node * make_node(const YAML::Node& node_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in)
{
    node::base::node * new_node = nullptr;
    auto node_yaml = node_yaml_in;
    auto template_node = node_yaml["template"];

    if (template_node) {
        assert(template_node.IsScalar());

        auto template_name = template_node.as<std::string>();
        auto template_src = config_in->get_parent()->get_template(template_name);
        apply_node_template(node_yaml, template_src);
    }

    // check for the name after applying the template so
    // the template might provide one
    auto node_name_node = node_yaml["name"];

    if (! node_name_node) {
        system_fault("node configuration did not include a name");
    }

    auto node_name = node_name_node.as<std::string>();

    auto class_name_node = node_yaml["class"];
    auto chain_name_node = node_yaml["chain"];

    if (class_name_node && chain_name_node) {
        system_fault("specify class or chain name but not both; node = ", node_name);
    } else if (class_name_node) {
        new_node = make_class_node(node_yaml, domain_in);
    } else if (chain_name_node) {
        auto chain_name = chain_name_node.as<std::string>();
        auto chain_node = config_in->get_parent()->get_chain(chain_name);
        new_node = make_chain_node(node_yaml, chain_node, config_in, domain_in);
    } else {
        system_fault("node did not have a class or chain name set; node = ", node_name);
    }

    assert(new_node != nullptr);
    return new_node;
}

std::map<std::string, pulsar::node::base::node *> make_nodes(std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in) {
    auto node_map = std::map<std::string, pulsar::node::base::node *>();

    for (YAML::Node node_yaml : config_in->get_nodes()) {
        auto new_node = make_node(node_yaml, config_in, domain_in);
        auto found = node_map.find(new_node->name);

        if (found != node_map.end()) {
            system_fault("duplicate node name: ", new_node->name);
        }

        node_map[new_node->name] = new_node;
    }

    for (auto&& node_yaml : config_in->get_nodes()) {
        connect_nodes(node_map, node_yaml);
    }

    return node_map;
}

file::file(const std::string& path_in)
: path(path_in)
{ }

void file::open()
{
    yaml_root = YAML::LoadFile(path);
    parse();
}

void file::parse()
{
    if (yaml_root["domain"] && yaml_root["domains"]) {
        system_fault("config file must have one domain or one domains section but not both");
    } else if (! yaml_root["domain"] && ! yaml_root["domains"]) {
        system_fault("config file must have a domain or a domains section");
    }
}

std::vector<std::string> file::get_domain_names()
{
    std::vector<std::string> retval;


    if (yaml_root["domain"]) {
        retval.push_back("main");
    } else if (yaml_root["domains"]) {
        system_fault("can't yet handle a domains section");
    }

    return retval;
}

std::shared_ptr<domain> file::get_domain(const std::string& name_in)
{
    if (name_in == "main" && yaml_root["domain"]) {
        return domain::make(name_in, yaml_root["domain"], this->shared_from_this());
    }

    auto domains = yaml_root["domains"];
    if (! domains[name_in]) {
        system_fault("could not find a domain named ", name_in);
    }

    return domain::make(name_in, domains[name_in], this->shared_from_this());
}

const YAML::Node file::get_templates()
{
    auto templates_node = yaml_root["templates"];

    if (! templates_node) {
        system_fault("there was not a template section in the config file");
    }

    if (! templates_node.IsMap()) {
        system_fault("template section must be a map");
    }

    return templates_node;
}

YAML::Node file::get_template(const std::string& name_in)
{

    auto template_node = get_templates()[name_in];

    if (! template_node) {
        system_fault("there was no node template named ", name_in);
    }

    if (! template_node.IsMap()) {
        system_fault("template must be a map: ", name_in);
    }

    return template_node;
}

const YAML::Node file::get_chains()
{
    auto chains_node = yaml_root["chains"];

    if (! chains_node) {
        system_fault("could not find a chains section");
    }

    if (! chains_node.IsMap()) {
        system_fault("chains section was not a map");
    }

    return chains_node;
}

const YAML::Node file::get_chain(const std::string& name_in)
{
    auto chain_node = get_chains()[name_in];

    if (! chain_node) {
        system_fault("could not find a chain named ", name_in);
    }

    return chain_node;
}

domain::domain(const std::string name_in, const YAML::Node& yaml_in, std::shared_ptr<file> parent_in)
: yaml_root(yaml_in), parent(parent_in), name(name_in)
{ }

std::shared_ptr<file> domain::get_parent()
{
    return parent;
}

const YAML::Node domain::get_config()
{
    assert(yaml_root["config"]);
    return yaml_root["config"];
}

const YAML::Node domain::get_nodes()
{
    assert(yaml_root["nodes"]);
    return yaml_root["nodes"];
}

} // namespace configfile

} // namespace pulsar

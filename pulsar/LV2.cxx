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

#include <pulsar/logging.h>
#include <pulsar/LV2.h>

namespace pulsar {

namespace LV2 {

static LilvWorld* lilv_world = nullptr;

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<LV2::node>(name_in);
}

void init()
{
    log_trace("Creating a new lilv world");
    lilv_world = lilv_world_new();

    if (lilv_world == nullptr) {
        system_fault("Could not create a new lilv world");
    }

    log_trace("Loading everything into the lilv world");
    lilv_world_load_all(lilv_world);

    library::register_node_factory("pulsar::LV2::node", make_node);
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::filter(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).value->set("pulsar::LV2::node");
    add_property("plugin:uri", property::value_type::string);
}

node::~node()
{
    if (instance != nullptr) {
        lilv_instance_free(instance);
        instance = nullptr;
    }

    if (urid_map_instance != nullptr) {
        free(urid_map_instance);
    }
}

void node::create_ports(const LilvPlugin * plugin_in)
{
    LilvNode * lv2_InputPort          = lilv_new_uri(lilv_world, LV2_CORE__InputPort);
    LilvNode * lv2_OutputPort         = lilv_new_uri(lilv_world, LV2_CORE__OutputPort);
    LilvNode * lv2_AudioPort          = lilv_new_uri(lilv_world, LV2_CORE__AudioPort);
    LilvNode * lv2_ControlPort        = lilv_new_uri(lilv_world, LV2_CORE__ControlPort);
    LilvNode * lv2_connectionOptional = lilv_new_uri(lilv_world, LV2_CORE__connectionOptional);
    size_type numports = lilv_plugin_get_num_ports(plugin_in);
    float * defaults = static_cast<float *>(calloc(numports, sizeof(float)));

    if (defaults == nullptr) {
        system_fault("could not malloc");
    }

    lilv_plugin_get_port_ranges_float(plugin_in, NULL, NULL, defaults);

    for(size_type i = 0; i < numports; i++) {
        const LilvPort * lport = lilv_plugin_get_port_by_index(plugin_in, i);
        auto lilv_port_name = lilv_port_get_name(plugin_in, lport);
        auto string_port_name = lilv_node_as_string(lilv_port_name);

        if (lilv_port_is_a(plugin_in, lport, lv2_AudioPort)) {
            if (lilv_port_is_a(plugin_in, lport, lv2_InputPort)) {
                audio.add_input(string_port_name);
            } else if (lilv_port_is_a(plugin_in, lport, lv2_OutputPort)) {
                audio.add_output(string_port_name);
            } else {
                system_fault("LV2 audio port was neither input nor output");
            }
        } else if (lilv_port_is_a(plugin_in, lport, lv2_ControlPort)) {
            string_type property_name;

            if (lilv_port_is_a(plugin_in, lport, lv2_InputPort)) {
                property_name = "config:";
            } else if (lilv_port_is_a(plugin_in, lport, lv2_OutputPort)) {
                property_name = "state:";
            } else {
                system_fault("LV2 audio port was neither input nor output");
            }

            property_name += string_port_name;

            auto& control = add_property(property_name, property::value_type::real);
            control.value->set(defaults[i]);
            // FIXME segfaults
            // lilv_instance_connect_port(instance, i, &control.value->get_real());
        } else {
            system_fault("LV2 port was neither audio nor control");
        }

        lilv_node_free(lilv_port_name);
    }

    free(defaults);
    lilv_node_free(lv2_connectionOptional);
    lilv_node_free(lv2_ControlPort);
    lilv_node_free(lv2_AudioPort);
    lilv_node_free(lv2_OutputPort);
    lilv_node_free(lv2_InputPort);
}

static LV2_URID urid_map_wrapper(LV2_URID_Map_Handle handle, const char *uri_in)
{
    auto node = (LV2::node *)handle;
    return node->urid_map_handler(uri_in);
}

LV2_URID node::urid_map_handler(const char *uri_in)
{
    string_type uri(uri_in);
    auto found = urid_map.find(uri);

    if (found != urid_map.end()) {
        return found->second;
    }

    auto next_urid = ++current_urid;
    urid_map[uri] = next_urid;
    return next_urid;
}

void node::init()
{
    assert(domain != nullptr);
    assert(instance == nullptr);

    urid_map_instance = static_cast<LV2_URID_Map *>(malloc(sizeof(LV2_URID_Map)));

    if (urid_map_instance == nullptr) {
        system_fault("could not malloc");
    }

    urid_map_instance->handle = this;
    urid_map_instance->map = urid_map_wrapper;

    auto string_uri = get_property("plugin:uri").value->get_string();

    if (string_uri == "") {
        system_fault("No LV2 URI was specified for node: ", name);
    }

    auto lilv_uri = lilv_new_uri(lilv_world, string_uri.c_str());
    if (! lilv_uri) {
        system_fault("Invalid plugin URI for node ", name, " : ", string_uri);
    }

    const LilvPlugins * plugins = lilv_world_get_all_plugins(lilv_world);
    const LilvPlugin * plugin  = lilv_plugins_get_by_uri(plugins, lilv_uri);

    if (plugin == nullptr) {
        system_fault("Could not find a LV2 plugin with URI of ", string_uri);
    }

    auto required_features = lilv_plugin_get_required_features(plugin);
    auto num_required = lilv_nodes_size(required_features);

    log_trace("LV2 required features for ", string_uri, ":");
    LILV_FOREACH(nodes, i, required_features) {
        auto node = lilv_nodes_get(required_features, i);
        auto value = lilv_node_as_string(node);
        log_trace("  ", value);
    }

    create_ports(plugin);

    LV2_Feature * feature_list[2];
    feature_list[0] = &urid_map_feature;
    feature_list[1] = nullptr;
    instance = lilv_plugin_instantiate(plugin, domain->sample_rate, feature_list);

    if (instance == nullptr) {
        system_fault("could not create LV2 instance");
    }

    lilv_node_free(lilv_uri);

    pulsar::node::filter::init();
}

void node::run()
{ }

} // namespace LV2

} // namespace pulsar
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

#pragma once

#include <pulsar/node.h>
#include <pulsar/library.h>
#include <pulsar/system.h>

namespace pulsar {

namespace LV2 {

#include <lilv/lilv.h>

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in);
void init();

class node : public pulsar::node::filter {

    protected:
    LilvInstance * instance = nullptr;
    LV2_URID_Map * urid_map_instance = nullptr;
    LV2_Feature urid_map_feature = {"http://lv2plug.in/ns/ext/urid#map", urid_map_instance};
    LV2_URID current_urid = 0;
    std::map<string_type, LV2_URID> urid_map;

    void create_ports(const LilvPlugin* plugin_in);
    virtual void init() override;
    virtual void run() override;

    public:
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual ~node();
    LV2_URID urid_map_handler(const char * uri_in);
    // virtual void activate() override;
};

} // namespace LV2

} // namespace pulsar

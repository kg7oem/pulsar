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

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <pulsar/audio.h>
#include <pulsar/dbus.h>
#include <pulsar/domain.h>
#include <pulsar/node.forward.h>
#include <pulsar/property.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace node {

void init();
string_type fully_qualify_property_name(const string_type& name_in);
base * make_chain_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);

struct dbus_node : public ::audio::pulsar::node_adaptor, public DBus::IntrospectableAdaptor, public DBus::ObjectAdaptor {
    base& parent;

    dbus_node(base& parent_in, const string_type& domain_name_in, const string_type& node_name_in);
    virtual std::vector<string_type> property_names() override;
    virtual std::string peek(const string_type& name_in) override;
};

struct base {
    friend void audio::component::source_ready(audio::input *);
    friend audio::input * audio::component::add_input(const string_type& name_in);
    friend audio::output * audio::component::add_output(const string_type& name_in);
    friend base * config::make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);
    // FIXME only run() is needed but run() is static and friend didn't like that
    friend pulsar::domain;
    friend dbus_node;

    protected:
    dbus_node dbus;
    mutex_type node_mutex;
    std::shared_ptr<pulsar::domain> domain;
    // FIXME pointer because I can't figure out how to make emplace() work
    std::map<string_type, property::generic *> properties;
    base(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in, const bool is_forwarder_in = false);

    /*
    * FIXME init_cycle should happen immediately before run but that does not
    * work right now for unknown reasons
    *
    * Typical node lifecycle
    *
    * construct
    * init
    * activate
    * start
    *
    * wait_inputs
    *
    * will_run
    *   init_cycle
    *   enqueue
    *
    * execute
    *   run
    *   did_run
    *   notify
    *   reset_cycle
    *
    * deactivate
    * deconstruct
    *
    */

    // needs to be reachable by templated factory methods
    public:
    virtual void activate();

    protected:
    virtual void start();
    virtual void init_cycle();
    virtual void will_run();
    virtual void run();
    virtual void did_run();
    virtual void notify();
    virtual void reset_cycle();
    virtual void deactivate();

    virtual void execute();
    property::generic& add_property(const string_type& name_in, const property::value_type& type_in);
    property::generic& add_property(const string_type& name_in, property::generic * property_in);

    public:
    const string_type name;
    const bool is_forwarder = false;
    audio::component audio;
    virtual ~base();
    std::shared_ptr<pulsar::domain> get_domain();
    const std::map<string_type, property::generic *>& get_properties();
    property::generic& get_property(const string_type& name_in);
    string_type peek(const string_type& name_in);
    virtual void init();
    virtual bool is_ready();
};

class forwarder : public base {
    protected:
    virtual void will_run() override;
    virtual void execute() override;
    virtual void notify() override;
    forwarder(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

struct chain : public forwarder {
    chain(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace node

} // namespace pulsar

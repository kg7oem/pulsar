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
#include <pulsar/domain.h>
#include <pulsar/node.forward.h>
#include <pulsar/property.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

#ifdef CONFIG_ENABLE_DBUS
#include <pulsar/dbus.h>
#endif

namespace pulsar {

namespace node {

    /*
    *
    * pulsar node lifecycle
    *
    * construct
    * init
    * activate
    * start
    *
    * input ready
    * init cycle
    * reset cycle
    *
    * stop
    * deactivate
    * deconstruct
    *
    *
    *
    * filter node cycle
    *
    * input ready
    *   init cycle
    *   enqueue
    * execute
    *   run
    *   notify
    *   reset_cycle
    *
    *
    *
    * io node cycle
    *
    * process (called by external thread and left blocking until later)
    *   init_cycle
    * input ready
    *   unblock caller
    * reset cycle
    *
    */

void init();
string_type fully_qualify_property_name(const string_type& name_in);
base * make_chain_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
size_type next_node_id();

#ifdef CONFIG_ENABLE_DBUS
struct dbus_node : public ::audio::pulsar::node_adaptor, public DBus::IntrospectableAdaptor, public DBus::ObjectAdaptor {
    base * parent;

    dbus_node(base * parent_in, const std::string& path_in);
    virtual std::vector<std::string> property_names() override;
    virtual std::map<std::string, std::string> properties() override;
    virtual std::string peek(const std::string& name_in) override;
    virtual void poke(const std::string& name_in, const std::string& value_in) override;
};
#endif

struct base {
    friend void audio::component::source_ready(audio::input *);
    friend audio::input * audio::component::add_input(const string_type& name_in);
    friend audio::output * audio::component::add_output(const string_type& name_in);
    friend base * config::make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);
    // FIXME only run() is needed but run() is static and friend didn't like that
    friend pulsar::domain;
#ifdef CONFIG_ENABLE_DBUS
    friend dbus_node;
#endif

    protected:
#ifdef CONFIG_ENABLE_DBUS
    std::list<dbus_node *> dbus_nodes{0, nullptr};
#endif
    mutex_type node_mutex;
    std::shared_ptr<pulsar::domain> domain;
    // FIXME pointer because I can't figure out how to make emplace() work
    std::map<string_type, property::property> properties;
    base(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in, const bool is_forwarder_in = false);
#ifdef CONFIG_ENABLE_DBUS
    void add_dbus(const std::string path_in);
#endif
    // needs to be reachable by templated factory methods
    public:
    virtual void activate();

    protected:
    virtual void start();
    virtual void init_cycle();
    virtual void input_ready() = 0;
    virtual void notify();
    virtual void reset_cycle();
    virtual void stop();
    virtual void deactivate();
    virtual void execute() = 0;

    property::property& add_property(const string_type& name_in, const property::value_type& type_in);
    property::property& add_property(const string_type& name_in, const property::property& property_in);

    public:
    const size_type id = next_node_id();
    const string_type name;
    const bool is_forwarder = false;
    audio::component audio;
    virtual ~base();
    std::shared_ptr<pulsar::domain> get_domain();
    const std::map<string_type, property::property>& get_properties();
    property::property& get_property(const string_type& name_in);
    string_type peek(const string_type& name_in);
    void poke(const string_type& name_in, const string_type& value_in);
    virtual void init();
    virtual bool is_ready();
};

class filter : public base {
    protected:
    filter(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual void execute() override;
    virtual void input_ready() override;
    virtual void run() = 0;
};

class io : public base {
    protected:
    std::condition_variable done_cond;
    mutex_type done_mutex;
    bool done_flag = false;

    io(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual void input_ready();
    virtual void unblock_caller();

    public:
    virtual void process(const std::map<string_type, sample_type *>& receives, const std::map<string_type, sample_type *>& sends);
};

class forwarder : public base {
    protected:
    virtual void input_ready() override;
    virtual void execute() override;
    virtual void notify() override;
    forwarder(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

struct chain : public forwarder {
    chain(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace node

} // namespace pulsar

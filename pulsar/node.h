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

#include "audio.h"
#include "domain.h"
#include "property.h"
#include "system.h"

namespace pulsar {

namespace config {

pulsar::node::base::node * make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);

} // namespace config

namespace node {

namespace base {

struct node;

} // namespace base

void init();
base::node * make_chain_node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
std::string fully_qualify_property_name(const std::string& name_in);

namespace base {

struct node {
    friend void audio::component::source_ready(audio::input *);
    friend audio::input * audio::component::add_input(const std::string& name_in);
    friend audio::output * audio::component::add_output(const std::string& name_in);
    friend node * config::make_chain_node(const YAML::Node& node_yaml_in, const YAML::Node& chain_yaml_in, std::shared_ptr<pulsar::config::domain> config_in, std::shared_ptr<pulsar::domain> domain_in);
    // FIXME only run() is needed but run() is static and friend didn't like that
    friend pulsar::domain;

    using mutex_type = std::mutex;
    using lock_type = std::unique_lock<mutex_type>;

    private:
    mutex_type mutex;

    protected:
    std::shared_ptr<pulsar::domain> domain;
    // FIXME pointer because I can't figure out how to make emplace() work
    std::map<std::string, property::generic *> properties;
    lock_type make_lock();

    /*
    * FIXME init_cycle should happen immediately before run but that does not
    * work right now for unknown reasons
    *
    * Typical node lifecycle
    *
    * construct
    * init
    * activate
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
    public: virtual void activate();
    protected:
    virtual void init_cycle();
    virtual void will_run();
    virtual void run();
    virtual void did_run();
    virtual void notify();
    virtual void reset_cycle();
    virtual void deactivate();

    virtual void execute();
    property::generic& add_property(const std::string& name_in, const property::value_type& type_in);
    property::generic& add_property(const std::string& name_in, property::generic * property_in);

    public:
    const std::string name;
    audio::component audio;
    node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual ~node();
    std::shared_ptr<pulsar::domain> get_domain();
    const std::map<std::string, property::generic *>& get_properties();
    property::generic& get_property(const std::string& name_in);
    std::string peek(const std::string& name_in);
    virtual void init();
    virtual bool is_ready();
};

} // namespace base

class chain : public base::node {
    protected:
    virtual void will_run() override;
    virtual void execute() override;
    virtual void notify() override;

    public:
    chain(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace node

} // namespace pulsar

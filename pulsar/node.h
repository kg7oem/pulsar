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

namespace node {

namespace base {

struct node;

} // namespace base

void init();
base::node * make_chain_node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
std::string fully_qualify_property_name(const std::string& name_in);

namespace base {

struct node {
    friend audio::input * audio::component::add_input(const std::string& name_in);
    friend audio::output * audio::component::add_output(const std::string& name_in);

    using mutex_type = std::mutex;
    using lock_type = std::unique_lock<mutex_type>;

    private:
    mutex_type mutex;

    protected:
    std::shared_ptr<pulsar::domain> domain;
    // FIXME pointer because I can't figure out how to make emplace() work
    std::map<std::string, property::generic *> properties;
    lock_type make_lock();
    virtual void handle_activate() = 0;
    virtual void handle_run();
    property::generic& add_property(const std::string& name_in, const property::value_type& type_in);

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
    void activate();
    void run();
    virtual void reset();
    virtual bool is_ready();
    virtual void handle_ready();
};

} // namespace base

class chain : public base::node {
    protected:
    virtual void handle_activate() override;

    public:
    chain(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace node

} // namespace pulsar

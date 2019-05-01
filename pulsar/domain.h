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
#include <condition_variable>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <pulsar/audio.h>
#include <pulsar/domain.forward.h>
#include <pulsar/node.forward.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

#ifdef CONFIG_HAVE_DBUS
#include <pulsar/dbus.h>
#endif

namespace pulsar {

const std::list<std::shared_ptr<domain>>& get_domains();
void add_domain(std::shared_ptr<domain>);

#ifdef CONFIG_HAVE_DBUS
struct dbus_node : public ::audio::pulsar::domain_adaptor, public DBus::IntrospectableAdaptor, public DBus::ObjectAdaptor {
    std::shared_ptr<domain> parent;

    dbus_node(std::shared_ptr<domain> parent_in);
    virtual std::string name() override;
};
#endif

struct domain : public std::enable_shared_from_this<domain> {
    private:
#ifdef CONFIG_HAVE_DBUS
    dbus_node * dbus = nullptr;
#endif
    std::shared_ptr<audio::buffer> zero_buffer = audio::buffer::make();
    std::vector<node::base *> nodes;
    bool activated = false;
    std::atomic<bool> is_online = ATOMIC_VAR_INIT(false);
    static void execute_one_node(node::base * node_in);

    public:
    const string_type name;
    const pulsar::size_type sample_rate;
    const pulsar::size_type buffer_size;
    template <typename... Args>
    static std::shared_ptr<domain> make(Args&&... args)
    {
        auto new_domain = std::make_shared<domain>(args...);
        new_domain->init();
        add_domain(new_domain);
        return new_domain;
    }
    domain(const string_type& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in);
    virtual ~domain();
    void init();
    void shutdown();
    std::shared_ptr<audio::buffer> get_zero_buffer();
    void activate();
    void step();
    void add_ready_node(node::base * node_in);
    void add_public_node(node::base * node_in);
    template<class T, typename... Args>
    T * make_node(Args&&... args)
    {
        auto new_node = new T(args..., this->shared_from_this());

        if (activated) {
            new_node->activate();
        }

        nodes.push_back(new_node);

        return new_node;
    }
};

} // namespace pulsar

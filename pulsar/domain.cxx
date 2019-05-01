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
#include <functional>

#include <pulsar/async.h>
#include <pulsar/debug.h>
#include <pulsar/domain.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>

namespace pulsar {

auto domain_list = new std::list<std::shared_ptr<domain>>();

const std::list<std::shared_ptr<domain>>& get_domains()
{
    return *domain_list;
}

void add_domain(std::shared_ptr<domain> domain_in)
{
    domain_list->push_back(domain_in);
}

#ifdef CONFIG_ENABLE_DBUS
// FIXME this should be in the domain namespace
static string_type make_dbus_path(const string_type& domain_name_in)
{
    return util::to_string(PULSAR_DBUS_DOMAIN_PREFIX, domain_name_in);
}

static string_type make_dbus_public_node_path(const string_type& domain_name_in, const string_type& node_name_in)
{
    return util::to_string(
        make_dbus_path(domain_name_in),
        PULSAR_DBUS_NODE_PREFIX,
        node_name_in
    );
}

dbus_node::dbus_node(std::shared_ptr<domain> parent_in)
:
    DBus::ObjectAdaptor(dbus::get_connection(), make_dbus_path(parent_in->name)),
    parent(parent_in)
{ }

std::string dbus_node::name()
{
    return parent->name;
}
#endif

domain::domain(const string_type& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in)
: name(name_in), sample_rate(sample_rate_in), buffer_size(buffer_size_in)
{
    // FIXME use mprotect() to set the zero_buffer memory as read-only
    // so it can't be accidently written over
    zero_buffer->init(buffer_size_in);
}

domain::~domain()
{
#ifdef CONFIG_ENABLE_DBUS
    if (dbus != nullptr) {
        delete dbus;
        dbus = nullptr;
    }
#endif
}

void domain::init()
{
#ifdef CONFIG_ENABLE_DBUS
    dbus = new dbus_node(this->shared_from_this());
#endif
}

void domain::shutdown()
{
    log_debug("shutting down domain ", name);
    if (! is_online) system_fault("can't shutdown a domain that is not online");

    is_online = false;

    for(auto&& node : nodes) {
        log_trace("stopping node ", node->name);
        node->stop();
    }
}

std::shared_ptr<audio::buffer> domain::get_zero_buffer()
{
    return zero_buffer;
}

void domain::activate()
{
    assert(! activated);

    activated = true;
    is_online = true;

    // nodes must be activated before they are started
    // but all nodes must be activated before any
    // are started
    for(auto&& node : nodes) {
        node->activate();
    }

    for(auto&& node : nodes) {
        node->start();
    }
}

void domain::add_ready_node(node::base * node_in)
{
    log_trace("adding ready node: ", node_in->name);

    assert(activated);

    if (! is_online) {
        log_trace("skipping adding a ready node for a domain that is not online");
    }

    async::submit_job(&domain::execute_one_node, node_in);
    log_trace("done adding ready node ", node_in->name);
}

void domain::add_public_node(node::base * node_in)
{
#ifdef CONFIG_ENABLE_DBUS
    node_in->add_dbus(make_dbus_public_node_path(name, node_in->name));
#endif
}

void domain::execute_one_node(node::base * node_in)
{
    log_trace("in execute_one_node() for ", node_in->name);
    node_in->execute();
    log_trace("done running node: ", node_in->name);
}

} // namespace pulsar

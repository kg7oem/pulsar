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
#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include <pulsar/async.h>
#include <pulsar/audio.util.h>
#include <pulsar/debug.h>
#include <pulsar/library.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>
#include <pulsar/util.h>

namespace pulsar {

namespace node {

static std::atomic<size_type> current_node_id = ATOMIC_VAR_INIT(0);

void init()
{
    library::register_node_factory("pulsar::node::chain", make_chain_node);
}

base * make_chain_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
{
    return domain_in->make_node<chain>(name_in);
}

size_type next_node_id()
{
    return ++current_node_id;
}

#ifdef CONFIG_ENABLE_DBUS
static std::string make_dbus_path(const std::string& name_in)
{
    return util::to_string(PULSAR_DBUS_NODE_PREFIX, name_in);
}

dbus_node::dbus_node(base * parent_in, const std::string& path_in)
:
    DBus::ObjectAdaptor(dbus::get_connection(), path_in),
    parent(parent_in)
{ }

std::vector<string_type> dbus_node::property_names()
{
    std::vector<string_type> retval;

    async::wait_job([this, &retval] {
        for (auto&& i : parent->properties) {
            retval.push_back(i.first);
        }
    });

    return retval;
}

std::map<std::string, std::string> dbus_node::properties()
{
    std::map<std::string, std::string> retval;

    async::wait_job([this, &retval] {
        for(auto&& i : parent->properties) {
            retval[i.first] = i.second.value->get();
        }
    });

    return retval;
}

string_type dbus_node::peek(const std::string& name_in)
{
    promise_type<std::string> promise;
    // FIXME find a way to turn this into an async function so wait_job() can return
    // a value through template args
    async::submit_job([this, &name_in, &promise] { promise.set_value(parent->peek(name_in)); });
    return promise.get_future().get();
}

void dbus_node::poke(const std::string& name_in, const std::string& value_in)
{
    async::wait_job([this, &name_in, &value_in] {
        parent->poke(name_in, value_in);
    });

    return;
}
#endif

base::base(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in, const bool is_forwarder_in)
: domain(domain_in), name(name_in), is_forwarder(is_forwarder_in), audio(this)
{
    assert(domain != nullptr);

    add_property("node:name", property::value_type::string).value->set(name);
    add_property("node:domain", pulsar::property::value_type::string).value->set(domain->name);
    add_property("node:id", pulsar::property::value_type::size).value->set(id);
}

base::~base()
{
#ifdef CONFIG_ENABLE_DBUS
    for(auto&& dbus : dbus_nodes) {
        assert(dbus != nullptr);
        delete dbus;
    }

    dbus_nodes.empty();
#endif
}

#ifdef CONFIG_ENABLE_DBUS
void base::add_dbus(const std::string path_in)
{
    dbus_nodes.push_back(new dbus_node(this, path_in));
}
#endif

std::shared_ptr<domain> base::get_domain()
{
    return domain;
}

property::property& base::get_property(const string_type& name_in)
{
    auto result = properties.find(name_in);

    if (result == properties.end()) {
        system_fault("no property existed with name: ", name_in);
    }

    return result->second;
}

string_type fully_qualify_property_name(const string_type& name_in)
{
    if (name_in.find(":") == string_type::npos) {
        return string_type("config:") + name_in;
    }

    return name_in;
}

string_type base::peek(const string_type& name_in)
{
    auto lock = debug_get_lock(node_mutex);
    auto name = fully_qualify_property_name(name_in);
    return get_property(name).value->get();
}

void base::poke(const string_type& name_in, const string_type& value_in)
{
    auto lock = debug_get_lock(node_mutex);
    auto name = fully_qualify_property_name(name_in);
    get_property(name).value->set(value_in);
}

const std::map<string_type, property::property>& base::get_properties()
{
    return properties;
}

property::property& base::add_property(const string_type& name_in, const property::value_type& type_in)
{
    if (properties.find(name_in) != properties.end()) system_fault("attempt to add duplicate property name: ", name_in);

    auto result = properties.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(name_in),
        std::forward_as_tuple(this, name_in, type_in)
    );

    return result.first->second;
}

property::property& base::add_property(const string_type& name_in, const property::property& property_in)
{
    if (properties.find(name_in) != properties.end()) system_fault("attempt to add duplicate property name: ", name_in);

    auto result = properties.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(name_in),
        std::forward_as_tuple(this, name_in, property_in.value)
    );

    return result.first->second;
}

void base::activate()
{
    log_debug("activating node ", name);

    audio.activate();
    reset_cycle();

    log_trace("done activating node ", name);
}

void base::start()
{ }

void base::init_cycle()
{
    log_trace("initializing cycle for node ", name);
    audio.init_cycle();
}

void base::notify()
{
    audio.notify();
}

void base::reset_cycle()
{
    audio.reset_cycle();
}

void base::stop()
{
    log_trace("node is stopped: ", name);
}

void base::deactivate()
{
    system_fault("cant deactivate yet");
}

void base::init()
{
#ifdef CONFIG_ENABLE_DBUS
    add_dbus(make_dbus_path(std::to_string(id)));
#endif
}

bool base::is_ready()
{
    return audio.is_ready();
}

filter::filter(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base(name_in, domain_in, false)
{ }

void filter::input_ready()
{
    init_cycle();
    domain->add_ready_node(this);
}

void filter::execute()
{
    log_debug("--------> node ", name, " started executing");
    auto lock = debug_get_lock(node_mutex);

    run();
    notify();
    reset_cycle();

    log_debug("<-------- node ", name, " finished executing");
}

io::io(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base(name_in, domain_in, true)
{ }

// FIXME this is called by a thread outside of Pulsar and could lead to
// priority inversion - this should be broken up into async jobs that
// run out of the job queue
void io::process(const std::map<string_type, sample_type *>& receives, const std::map<string_type, sample_type *>& sends)
{
    log_trace("IO node process() was just invoked");

    auto node_lock = debug_get_lock(node_mutex);
    auto done_lock = debug_get_lock(done_mutex);

    if (done_flag) {
        system_fault("IO node process() went reentrant");
    }

    done_lock.unlock();

    init_cycle();

    log_trace("IO node is setting up output buffers");
    for(auto&& name : audio.get_output_names()) {
        auto output = audio.get_output(name);
        auto user_buffer = receives.find(name);
        auto buffer = audio::buffer::make();

        if (user_buffer == receives.end()) {
            system_fault("could not find user supplied buffer for IO output: ", name);
        }

        buffer->init(domain->buffer_size, user_buffer->second);
        output->set_buffer(buffer);
    }

    node_lock.unlock();

    log_trace("waiting for IO node to become done");
    debug_relock(done_lock);
    done_cond.wait(done_lock, [this]{ return done_flag; });
    done_flag = false;
    log_trace("IO node is now done");

    debug_relock(node_lock);

    for(auto&& name : audio.get_input_names()) {
        auto buffer_size = domain->buffer_size;
        auto input = audio.get_input(name);
        auto user_buffer = sends.find(name);
        auto channel_buffer = input->get_buffer();

        if (user_buffer == sends.end()) {
            system_fault("could not find user supplied buffer for IO input: ", name);
        }

        audio::util::pcm_set(user_buffer->second, channel_buffer->get_pointer(), buffer_size);
    }

    reset_cycle();
}

void io::input_ready()
{
    log_trace("IO node has all inputs ready; unblocking caller");
    unblock_caller();
}

void io::unblock_caller()
{
    log_trace("waking up blocked IO node thread");
    auto done_lock = debug_get_lock(done_mutex);
    done_flag = true;
    done_cond.notify_all();
}

forwarder::forwarder(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base(name_in, domain_in, true)
{ }

void forwarder::input_ready()
{
    log_trace("forwarder node is short-circuting execute(): ", name);

    auto lock = debug_get_lock(node_mutex);

    // a forwarder node does not use any CPU since all inputs and outputs
    // are forwarded but a full cycle still needs to happen so the
    // ready node queue can be skipped
    init_cycle();
    reset_cycle();
}

void forwarder::execute()
{
    system_fault("forwarder nodes should never try to execute");
}

// notifications happen via forwarding
void forwarder::notify()
{
    system_fault("forwarder nodes should never try to notify");
}

chain::chain(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: forwarder(name_in, domain_in)
{ }

} // namespace node

} // namespace pulsar

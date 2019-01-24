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
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <pulsar/audio.h>
#include <pulsar/audio.util.h>
#include <pulsar/debug.h>
#include <pulsar/logging.h>
#include <pulsar/node.h>
#include <pulsar/system.h>

#define PULSAR_SANITY_CHECK_WAITING
#define PULSAR_SANITY_CHECK_WAITING_LIMIT 1000000

namespace pulsar {

audio::buffer::~buffer()
{
    if (own_memory) {
        assert(pointer != nullptr);
        std::free(pointer);
        pointer = nullptr;
    }
}

void audio::buffer::init(const pulsar::size_type buffer_size_in, pulsar::sample_type * pointer_in)
{
    assert(pointer == nullptr);

    size = buffer_size_in;

    if (pointer_in != nullptr) {
        pointer = pointer_in;
    } else {
        own_memory = true;
        pointer = static_cast<pulsar::sample_type *>(std::calloc(size, sizeof(pulsar::sample_type)));

        if (pointer == nullptr) {
            system_fault("could not allocate memory for audio buffer");
        }
    }

    return;
}

pulsar::size_type audio::buffer::get_size()
{
    assert(pointer != nullptr);
    return size;
}

pulsar::sample_type * audio::buffer::get_pointer()
{
    assert(pointer != nullptr);
    return pointer;
}

void audio::buffer::zero()
{
    assert(pointer != nullptr);
    audio::util::pcm_zero(pointer, size);
}

void audio::buffer::mix(std::shared_ptr<audio::buffer> mix_from_in)
{
    assert(pointer != nullptr);

    if (size != mix_from_in->size) {
        system_fault("attempt to mix buffers of different size");
    }

    audio::util::pcm_mix(pointer, mix_from_in->get_pointer(), size);
}

void audio::buffer::set(pulsar::sample_type * pointer_in, const size_type size_in)
{
    assert(pointer != nullptr);

    if (size_in > size) {
        system_fault("attempt to set buffer contents with a size that was too large");
    }

    audio::util::pcm_set(pointer, pointer_in, size_in);
}

void audio::buffer::set(std::shared_ptr<audio::buffer> buffer_in)
{
    if (size != buffer_in->size) {
        system_fault("attempt to set buffer contents from buffer of different size");
    }

    auto src_p = buffer_in->get_pointer();
    set(src_p, size);
}

void audio::buffer::scale(const float scale_in)
{
    assert(pointer != nullptr);
    audio::util::pcm_scale(pointer, scale_in, size);
}

audio::channel::channel(const string_type &name_in, node::base * parent_in)
: parent(parent_in), name(name_in)
{ }

audio::channel::~channel()
{ }

void audio::channel::register_link(link * link_in)
{
    links.push_back(link_in);
}

node::base * audio::channel::get_parent()
{
    return parent;
}

audio::input::input(const string_type& name_in, node::base * parent_in)
: audio::channel(name_in, parent_in)
{ }

void audio::input::init_cycle()
{ }

void audio::input::reset_cycle()
{
    {
        auto lock = debug_get_lock(link_buffers_mutex);
        link_buffers.empty();
    }

    for(auto&& link : links) {
        link->reset();
    }

    auto waiting_things = links.size() + num_forwards_to_us;
    llog_trace({ return pulsar::util::to_string("resetting audio input ", to_string()); });

    links_waiting.store(waiting_things);
}

void audio::input::link_to(audio::output * source_in) {
    auto new_link = new audio::link(source_in, this);
    register_link(new_link);
    source_in->register_link(new_link);
}

void audio::input::link_to(node::base * node_in, const string_type& port_name_in)
{
    if (port_name_in == "*") {
        for(auto&& output_name : node_in->audio.get_output_names()) {
            assert(output_name != "*");
            link_to(node_in, output_name);
        }
    } else {
        auto output = node_in->audio.get_output(port_name_in);
        link_to(output);
    }
}

void audio::input::forward_to(input * to_in)
{
    if (! parent->is_forwarder) {
        system_fault("node to forward from is not a forwarder: ", parent->name);
    }

    if (to_in->get_parent()->is_forwarder) {
        system_fault("node to forward to is also a forwarder: ", parent->name, ":", name, " -> ", to_in->get_parent()->name, ":", to_in->name);
    }

    auto new_forward = new audio::input_forward(this, to_in);
    forwards.push_back(new_forward);
    to_in->register_forward(new_forward);
}

void audio::input::forward_to(node::base * node_in, const string_type& port_name_in)
{
    if (port_name_in == "*") {
        for(auto&& input_name : node_in->audio.get_input_names()) {
            assert(input_name != "*");
            forward_to(node_in, input_name);
        }
    } else {
        auto input = node_in->audio.get_input(port_name_in);
        forward_to(input);
    }
}

void audio::input::link_ready(audio::link * link_in, std::shared_ptr<audio::buffer> buffer_in)
{
    llog_trace({ return pulsar::util::to_string("in link_ready() for ", to_string()); });

    assert(link_in != nullptr);
    assert(buffer_in != nullptr);

    size_type now_waiting;

    {
        auto lock = debug_get_lock(link_buffers_mutex);
        link_buffers[link_in] = buffer_in;
    }

    now_waiting = --links_waiting;

    llog_trace({ return pulsar::util::to_string("waiting buffers: ", now_waiting, "; node: ", get_parent()->name); });

#ifdef PULSAR_SANITY_CHECK_WAITING
    if (now_waiting > PULSAR_SANITY_CHECK_WAITING_LIMIT) system_fault("sanity check failed; waiting for ", now_waiting, "for node", parent->name);
#endif

    if (now_waiting == 0) {
        log_trace(to_string(), " telling audio component we are ready");
        parent->audio.source_ready(this);
    }

    for(auto&& forward : forwards) {
        llog_trace({ return pulsar::util::to_string("node ", parent->name, " forwarding to ", forward->to->get_parent()->name, ":", forward->to->name); });
        forward->to->link_ready(link_in, buffer_in);
    }
}

pulsar::size_type audio::input::get_links_waiting()
{
    return links_waiting.load();
}

void audio::input::register_forward(UNUSED input_forward * forward_in)
{
    num_forwards_to_us++;
}

// if there are no links in the input channel then this
// returns a pointer to a buffer that is full of 0
// value samples
//
// if there is only one link for the input channel then this
// returns a pointer to the buffer in the linked output channel
//
// if there is more than one link in the input channel then
// returns a pointer to the internal buffer after it is used to
// sum all the buffers from the linked output channels
std::shared_ptr<audio::buffer> audio::input::get_buffer()
{
    auto num_links = links.size() + num_forwards_to_us;
    auto input_name = parent->name + ":" + name;

    if (num_links == 0) {
        log_trace("returning pointer to zero buffer for ", input_name);
        return parent->get_domain()->get_zero_buffer();
    } else if (num_links == 1) {
        log_trace("returning pointer to link's ready buffer for ", input_name);
        auto lock = debug_get_lock(link_buffers_mutex);
        assert(link_buffers.begin() != link_buffers.end());
        assert(link_buffers.begin()->second != nullptr);
        return link_buffers.begin()->second;
    } else {
        log_trace("returning pointer to mix buffer for", input_name);
        return mix_outputs();
    }
}

std::shared_ptr<audio::buffer> audio::input::mix_outputs()
{
    llog_trace({ return pulsar::util::to_string("mixing ", links.size(), " input buffers for ", parent->name, ":", name); });

    assert(links.size() + num_forwards_to_us > 1);

    auto mix_buffer = std::make_shared<audio::buffer>();
    mix_buffer->init(parent->get_domain()->buffer_size);

    for(auto&& buffer : link_buffers) {
        mix_buffer->mix(buffer.second);
    }

    return mix_buffer;
}

const std::string audio::input::to_string()
{
    string_type buf = parent->name;
    buf += ":input:" + name;
    return buf;
}

audio::output::output(const string_type& name_in, node::base * parent_in)
: audio::channel(name_in, parent_in)
{ }

void audio::output::init_cycle()
{
    llog_trace({ return pulsar::util::to_string("starting cycle for ", to_string()); });
    buffer = std::make_shared<audio::buffer>();
    buffer->init(parent->get_domain()->buffer_size);
}

void audio::output::reset_cycle()
{
    llog_trace({ return pulsar::util::to_string("ending cycle for ", to_string()); });
    buffer = nullptr;
}

void audio::output::register_forward(UNUSED output_forward * forward_in)
{ }

std::shared_ptr<audio::buffer> audio::output::get_buffer()
{
    assert(buffer != nullptr);
    return buffer;
}

void audio::output::set_buffer(std::shared_ptr<audio::buffer> buffer_in)
{
    assert(buffer_in != nullptr);

    buffer = buffer_in;
    notify();
}

void audio::output::link_to(audio::input * sink_in)
{
    auto new_link = new audio::link(this, sink_in);
    register_link(new_link);
    sink_in->register_link(new_link);
}

void audio::output::link_to(node::base * node_in, const string_type& port_name_in)
{
    if (port_name_in == "*") {
        for(auto&& input_name : node_in->audio.get_input_names()) {
            assert(input_name != "*");
            link_to(node_in, input_name);
        }
    } else {
        auto input = node_in->audio.get_input(port_name_in);
        link_to(input);
    }
}

void audio::output::forward_to(output * to_in)
{
    if (parent->is_forwarder) {
        system_fault("node to forward output from is a forwarder: ", parent->name);
    }

    if (! to_in->get_parent()->is_forwarder) {
        system_fault("node to forward output to is not a forwarder: ", parent->name, ":", name, " -> ", to_in->get_parent()->name, ":", to_in->name);
    }

    auto new_forward = new audio::output_forward(this, to_in);
    forwards.push_back(new_forward);
    to_in->register_forward(new_forward);
}

void audio::output::forward_to(node::base * node_in, const string_type& port_name_in)
{
    if (port_name_in == "*") {
        for(auto&& output_name : node_in->audio.get_output_names()) {
            assert(output_name != "*");
            link_to(node_in, output_name);
        }
    } else {
        auto output = node_in->audio.get_output(port_name_in);
        forward_to(output);
    }
}

void audio::output::notify()
{
    // FIXME make an assert macro for this
    if (buffer == nullptr) {
        system_fault("buffer was null for ", parent->name, ":", name);
    }

    for(auto&& forward : forwards) {
        forward->to->set_buffer(buffer);
    }

    for(auto&& link : links) {
        link->notify(buffer);
    }
}

const std::string audio::output::to_string()
{
    string_type buf = parent->name;
    buf += ":output:" + name;
    return buf;
}

audio::link::link(audio::output * from_in, audio::input * to_in)
: from(from_in), to(to_in)
{

}

void audio::link::reset()
{
    llog_trace({ return pulsar::util::to_string("resetting link for ",  to_string()); });

    // FIXME does this need a lock? The flag is atomic
    // and locks are not needed to wake up a condvar
    auto lock = debug_get_lock(available_mutex);
    available_flag = true;
    available_condition.notify_all();
}

void audio::link::notify(std::shared_ptr<audio::buffer> ready_buffer_in, const bool blocking_in)
{
    llog_trace({ return pulsar::util::to_string("got notification for ", to_string()); });

    auto lock = debug_get_lock(available_mutex);

    if (! available_flag) {
        if (blocking_in) {
            llog_trace({ return pulsar::util::to_string("node is blocked on link ", to_string()); });
            available_condition.wait(lock, [this]{ return available_flag.load(); });
        } else {
            system_fault("attempt to set link ready when it was already ready");
        }
    }

    available_flag = false;

    lock.unlock();

    to->link_ready(this, ready_buffer_in);
}

const std::string audio::link::to_string()
{
    auto buf = from->to_string();
    buf += " -> " + to->to_string();
    return buf;
}


audio::input_forward::input_forward(input * from_in, input * to_in)
: from(from_in), to(to_in)
{ }

audio::output_forward::output_forward(output * from_in, output * to_in)
: from(from_in), to(to_in)
{ }

audio::component::component(node::base * parent_in)
: parent(parent_in)
{

}

audio::component::~component()
{
    for (auto&& i : inputs) {
        delete i.second;
    }

    for(auto&& i : outputs) {
        delete i.second;
    }

    inputs.clear();
    outputs.clear();
}

void audio::component::init_cycle()
{
    log_trace("audio component is starting cycle for node ", parent->name);

    for(auto&& output : outputs) {
        output.second->init_cycle();
    }

    for(auto&& input : inputs) {
        input.second->init_cycle();
    }
}

void audio::component::reset_cycle()
{
    log_trace("audio component is ending cycle for node ", parent->name);

    pulsar::size_type inputs_with_links = 0;

    for(auto&& output : outputs) {
        output.second->reset_cycle();
    }

    for(auto&& input : inputs) {
        input.second->reset_cycle();

        if (input.second->get_links_waiting() > 0) {
            inputs_with_links++;
        }
    }

    inputs_waiting.store(inputs_with_links);
}

pulsar::size_type audio::component::get_inputs_waiting()
{
    return inputs_waiting.load();
}

bool audio::component::is_ready()
{
    return get_inputs_waiting() == 0;
}

void audio::component::activate()
{ }

void audio::component::notify()
{
    for (auto&& output : outputs) {
        output.second->notify();
    }
}

void audio::component::source_ready(audio::input *)
{
    // FIXME this should signal to the node that the component
    // is ready instead of running the parent

    auto now_waiting = --inputs_waiting;
    log_trace("node ", parent->name, " audio sources now waiting: ", now_waiting);

#ifdef PULSAR_SANITY_CHECK_WAITING
    if (now_waiting > PULSAR_SANITY_CHECK_WAITING_LIMIT) {
        system_fault("sanity check failed: links now waiting is ", now_waiting, "for node ", parent->name);
    }
#endif

    if (now_waiting == 0) {
        // FIXME RACE is this the race condition causing deadlocks?
        assert(parent->is_ready());
        parent->will_run();
    }
}

audio::input * audio::component::add_input(const string_type& name_in)
{
    if (inputs.count(name_in) != 0) {
        system_fault("attempt to add duplicate input name: " + name_in);
    }

    auto new_input = new audio::input(name_in, parent);
    inputs[new_input->name] = new_input;

    auto property_name = string_type("input:") + name_in;
    parent->add_property(property_name, property::value_type::string).set("audio");

    return new_input;
}

audio::input * audio::component::get_input(const string_type& name_in)
{
    if (inputs.count(name_in) == 0) {
        system_fault("could not find input channel named ", name_in, " for node ", parent->name);
    }

    return inputs[name_in];
}

std::vector<string_type> audio::component::get_input_names()
{
    std::vector<string_type> retval;

    for(auto&& input : inputs) {
        retval.push_back(input.first);
    }

    return retval;
}

audio::output * audio::component::add_output(const string_type& name_in)
{
    if (outputs.count(name_in) != 0) {
        system_fault("attempt to add duplicate output name: " + name_in);
    }

    auto new_output = new audio::output(name_in, parent);
    outputs[new_output->name] = new_output;

    auto property_name = string_type("output:") + name_in;
    parent->add_property(property_name, property::value_type::string).set("audio");

    return new_output;
}

audio::output * audio::component::get_output(const string_type& name_in)
{
    if (outputs.count(name_in) == 0) {
        system_fault("could not find output channel named ", name_in, " for node ", parent->name);
    }

    return outputs[name_in];
}

std::vector<string_type> audio::component::get_output_names()
{
    std::vector<string_type> retval;

    for(auto&& output : outputs) {
        retval.push_back(output.first);
    }

    return retval;
}

} // namespace pulsar

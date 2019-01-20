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

#include "audio.h"
#include "audio.util.h"
#include "logging.h"
#include "node.h"

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
            throw std::runtime_error("could not allocate memory for audio buffer");
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
        throw std::runtime_error("attempt to mix buffers of different size");
    }

    audio::util::pcm_mix(mix_from_in->get_pointer(), pointer, size);
}

void audio::buffer::set(pulsar::sample_type * pointer_in, const size_type size_in)
{
    assert(pointer != nullptr);

    if (size_in > size) {
        throw std::runtime_error("attempt to set buffer contents with a size that was too large");
    }

    audio::util::pcm_set(pointer, pointer_in, size_in);
}

void audio::buffer::set(std::shared_ptr<audio::buffer> buffer_in)
{
    if (size != buffer_in->size) {
        throw std::runtime_error("attempt to set buffer contents from buffer of different size");
    }

    auto src_p = buffer_in->get_pointer();
    set(src_p, size);
}

void audio::buffer::scale(const float scale_in)
{
    assert(pointer != nullptr);
    audio::util::pcm_scale(pointer, scale_in, size);
}

audio::channel::channel(const std::string &name_in, node::base::node * parent_in)
: parent(parent_in), name(name_in)
{ }

audio::channel::~channel()
{ }

// FIXME this should call reset() to create the buffer
void audio::channel::activate()
{
    // buffer = std::make_shared<audio::buffer>();
    // buffer->init(parent->get_domain()->buffer_size);
}

void audio::channel::init_cycle()
{ }

void audio::channel::reset_cycle()
{ }

void audio::channel::add_link(link * link_in)
{
    links.push_back(link_in);
}

node::base::node * audio::channel::get_parent()
{
    return parent;
}

audio::input::input(const std::string& name_in, node::base::node * parent_in)
: audio::channel(name_in, parent_in)
{

}

void audio::input::reset_cycle()
{
    {
        auto lock = lock_type(link_buffers_mutex);
        link_buffers.empty();
    }

    for(auto&& link : links) {
        link->reset();
    }

    auto waiting_things = links.size() + num_forwards_to_us;
    log_trace("resetting audio input ", parent->name, ":", name, "; waiting things: ", waiting_things);

    links_waiting.store(waiting_things);
}

void audio::input::connect(audio::output * source_in) {
    auto new_link = new audio::link(source_in, this);
    add_link(new_link);
    source_in->add_link(new_link);
}

void audio::input::forward(input * to_in)
{
    auto new_forward = new audio::input_forward(this, to_in);
    forwards.push_back(new_forward);
    to_in->add_forward(new_forward);
}

void audio::input::link_ready(link * link_in, std::shared_ptr<audio::buffer> buffer_in)
{
    log_trace("in link_ready() for ", parent->name, ":", name);

    assert(link_in != nullptr);
    assert(buffer_in != nullptr);

    size_type now_waiting;

    {
        auto lock = lock_type(link_buffers_mutex);
        link_buffers[link_in] = buffer_in;
    }

    now_waiting = --links_waiting;

    log_trace("waiting buffers: ", now_waiting, "; node: ", get_parent()->name);

    if (now_waiting == 0) {
        parent->audio.source_ready(this);
    }

    for(auto&& forward : forwards) {
        log_trace("node ", parent->name, " forwarding to ", forward->to->get_parent()->name, ":", forward->to->name);
        forward->to->link_ready(link_in, buffer_in);
    }
}

pulsar::size_type audio::input::get_links_waiting()
{
    return links_waiting.load();
}

void audio::input::add_forward(UNUSED input_forward * forward_in)
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
        auto lock = lock_type(link_buffers_mutex);
        assert(link_buffers.begin() != link_buffers.end());
        assert(link_buffers.begin()->second != nullptr);
        return link_buffers.begin()->second;
    } else {
        log_trace("returning pointer to input's mix buffer for", input_name);
        return mix_sinks();
    }
}

std::shared_ptr<audio::buffer> audio::input::mix_sinks()
{
    assert(links.size() + num_forwards_to_us > 1);

    auto mix_buffer = std::make_shared<audio::buffer>();
    mix_buffer->init(parent->get_domain()->buffer_size);

    for(auto&& buffer : link_buffers) {
        mix_buffer->mix(buffer.second);
    }

    return mix_buffer;
}

audio::output::output(const std::string& name_in, node::base::node * parent_in)
: audio::channel(name_in, parent_in)
{

}

void audio::output::init_cycle()
{
    log_trace("creating output buffer for ", parent->name, ":", name);

    buffer = std::make_shared<audio::buffer>();
    buffer->init(parent->get_domain()->buffer_size);

    audio::channel::init_cycle();
}

void audio::output::reset_cycle()
{
    buffer = nullptr;
}

void audio::output::add_forward(UNUSED output_forward * forward_in)
{
    // forwards.push_back(forward_in);
}

std::shared_ptr<audio::buffer> audio::output::get_buffer()
{
    return parent->get_domain()->get_zero_buffer();

    // if (buffer == nullptr) {
    //     return parent->get_domain()->get_zero_buffer();
    // }

    // buffer = std::make_shared<audio::buffer>();
    // buffer->init(parent->get_domain()->buffer_size);

    // return buffer;
}

void audio::output::set_buffer(std::shared_ptr<audio::buffer> buffer_in, const bool notify_in)
{
    buffer = buffer_in;

    if (notify_in) {
        notify();
    }
}

void audio::output::connect(audio::input * sink_in)
{
    auto new_link = new audio::link(this, sink_in);
    add_link(new_link);
    sink_in->add_link(new_link);
}

void audio::output::forward(output * to_in)
{
    auto new_forward = new audio::output_forward(this, to_in);
    forwards.push_back(new_forward);
    to_in->add_forward(new_forward);
}

void audio::output::notify(std::shared_ptr<audio::buffer> buffer_in)
{

    if (buffer_in != nullptr) {
        buffer = buffer_in;
    }

    assert(buffer != nullptr);

    for(auto&& forward : forwards) {
        forward->to->notify(buffer);
    }

    for(auto&& link : links) {
        link->notify(buffer);
    }
}

audio::link::link(audio::output * sink_in, audio::input * source_in)
: sink(sink_in), source(source_in)
{

}

void audio::link::reset()
{
    // FIXME does this need a lock? The flag is atomic
    // and locks are needed to wake up a condvar
    auto lock = lock_type(available_mutex);
    log_trace("resetting link");
    available_flag = true;
    available_condition.notify_all();
}

void audio::link::notify(std::shared_ptr<audio::buffer> ready_buffer_in, const bool blocking_in)
{
    log_trace("got notification for ", sink->get_parent()->name, ":", sink->name);

    auto lock = lock_type(available_mutex);

    if (! available_flag) {
        if (blocking_in) {
            log_debug("node ", source->get_parent()->name, ":", source->name, " is blocked notifying ", sink->get_parent()->name, ":", sink->name);
            available_condition.wait(lock, [this]{ return available_flag.load(); });
        } else {
            throw std::runtime_error("attempt to set link ready when it was already ready");
        }
    }

    available_flag = false;

    lock.unlock();

    source->link_ready(this, ready_buffer_in);
}

audio::input_forward::input_forward(input * from_in, input * to_in)
: from(from_in), to(to_in)
{ }

audio::output_forward::output_forward(output * from_in, output * to_in)
: from(from_in), to(to_in)
{ }

audio::component::component(node::base::node * parent_in)
: parent(parent_in)
{

}

audio::component::~component()
{
    for (auto&& i : sources) {
        delete i.second;
    }

    for(auto&& i : sinks) {
        delete i.second;
    }

    sources.clear();
    sinks.clear();
}

void audio::component::init_cycle()
{
    for(auto&& output : sinks) {
        output.second->init_cycle();
    }

    for(auto&& input : sources) {
        input.second->init_cycle();
    }
}

void audio::component::reset_cycle()
{
    pulsar::size_type inputs_with_links = 0;

    for(auto&& output : sinks) {
        output.second->reset_cycle();
    }

    for(auto&& input : sources) {
        input.second->reset_cycle();

        if (input.second->get_links_waiting() > 0) {
            inputs_with_links++;
        }
    }

    sources_waiting.store(inputs_with_links);
}

pulsar::size_type audio::component::get_sources_waiting()
{
    return sources_waiting.load();
}

bool audio::component::is_ready()
{
    return get_sources_waiting() == 0;
}

void audio::component::activate()
{
    for (auto&& output : sinks) {
        output.second->activate();
    }

    for(auto&& input : sources) {
        input.second->activate();
    }
}

void audio::component::notify()
{
    for (auto&& output : sinks) {
        output.second->notify();
    }
}

void audio::component::source_ready(audio::input *)
{
    if (--sources_waiting == 0 && parent->is_ready()) {
        parent->do_ready();
    }
}

audio::input * audio::component::add_input(const std::string& name_in)
{
    if (sources.count(name_in) != 0) {
        throw std::runtime_error("attempt to add duplicate input name: " + name_in);
    }

    auto new_input = new audio::input(name_in, parent);
    sources[new_input->name] = new_input;

    auto property_name = std::string("input:") + name_in;
    parent->add_property(property_name, property::value_type::string).set("audio");

    return new_input;
}

audio::input * audio::component::get_input(const std::string& name_in)
{
    if (sources.count(name_in) == 0) {
        system_fault("could not find input channel named ", name_in, " for node ", parent->name);
    }

    return sources[name_in];
}

std::vector<std::string> audio::component::get_input_names()
{
    std::vector<std::string> retval;

    for(auto&& input : sources) {
        retval.push_back(input.first);
    }

    return retval;
}

audio::output * audio::component::add_output(const std::string& name_in)
{
    if (sinks.count(name_in) != 0) {
        throw std::runtime_error("attempt to add duplicate output name: " + name_in);
    }

    auto new_output = new audio::output(name_in, parent);
    sinks[new_output->name] = new_output;

    auto property_name = std::string("output:") + name_in;
    parent->add_property(property_name, property::value_type::string).set("audio");

    return new_output;
}

audio::output * audio::component::get_output(const std::string& name_in)
{
    if (sinks.count(name_in) == 0) {
        system_fault("could not find output channel named ", name_in, " for node ", parent->name);
    }

    return sinks[name_in];
}

std::vector<std::string> audio::component::get_output_names()
{
    std::vector<std::string> retval;

    for(auto&& output : sinks) {
        retval.push_back(output.first);
    }

    return retval;
}

} // namespace pulsar

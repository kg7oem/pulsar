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
#include <iostream>
#include <stdexcept>

#include "audio.h"
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

void audio::buffer::init(const pulsar::size_type buffer_size_in)
{
    assert(pointer == nullptr);

    size = buffer_size_in;

    if (own_memory) {
        pointer = static_cast<pulsar::sample_type *>(std::calloc(size, sizeof(pulsar::sample_type)));

        if (pointer == nullptr) {
            throw std::runtime_error("could not allocate memory for audio buffer");
        }
    }

    return;
}

pulsar::size_type audio::buffer::get_size()
{
    return size;
}

pulsar::sample_type * audio::buffer::get_pointer()
{
    assert(pointer != nullptr);
    return pointer;
}

// void audio::buffer::set_pointer(pulsar::sample_type * pointer_in)
// {
//     assert(own_memory == false);
//     pointer = pointer_in;
// }

// void audio::buffer::clear_pointer()
// {
//     set_pointer(nullptr);
// }

// void audio::buffer::release_memory()
// {
//     assert(own_memory == true);
//     free(pointer);
//     pointer = nullptr;
//     own_memory = false;
// }

void audio::buffer::zero()
{
    assert(pointer != nullptr);
    std::memset(pointer, 0, size);
}

void audio::buffer::mix(std::shared_ptr<audio::buffer> mix_from_in)
{
    if (size != mix_from_in->size) {
        throw std::runtime_error("attempt to mix buffers of different size");
    }

    auto src_p = mix_from_in->get_pointer();

    for(size_type i = 0; i < size; i++) {
        pointer[i] += src_p[i];
    }
}

void audio::buffer::set(pulsar::sample_type * pointer_in, const size_type size_in)
{
    if (size_in > size) {
        throw std::runtime_error("attempt to set buffer contents with a size that was too large");
    }

    for(size_type i = 0; i < size_in; i++) {
        pointer[i] = pointer_in[i];
    }
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
    for(size_type i = 0; i < size; i++) {
        pointer[i] /= scale_in;
    }
}

audio::channel::channel(const std::string &name_in, pulsar::node * parent_in)
: parent(parent_in), name(name_in)
{ }

audio::channel::~channel()
{ }

void audio::channel::activate()
{
    buffer = std::make_shared<audio::buffer>();
    buffer->init(parent->get_domain()->buffer_size);
}

void audio::channel::add_link(link * link_in)
{
    links.push_back(link_in);
}

node * audio::channel::get_parent()
{
    return parent;
}

std::shared_ptr<audio::buffer> audio::channel::get_buffer()
{
    return buffer;
}

audio::input::input(const std::string& name_in, node * parent_in)
: audio::channel(name_in, parent_in)
{

}

void audio::input::connect(audio::output * source_in) {
    auto new_link = new audio::link(source_in, this);
    add_link(new_link);
    source_in->add_link(new_link);
}

void audio::input::link_ready(link *)
{
    if (--links_waiting == 0) {
        parent->audio.source_ready(this);
    }
}

pulsar::size_type audio::input::get_links_waiting()
{
    return links_waiting.load();
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
pulsar::sample_type * audio::input::get_pointer()
{
    auto num_links = links.size();

    if (num_links == 0) {
        return parent->get_domain()->get_zero_buffer().get_pointer();
    } else if (num_links == 1) {
        return links[0]->get_ready_buffer()->get_pointer();
    } else {
        mix_sinks();
        return buffer->get_pointer();
    }
}

void audio::input::reset()
{
    for(auto link : links) {
        link->reset();
    }

    links_waiting.store(links.size());
}

void audio::input::mix_sinks()
{
    buffer->zero();

    for(auto link : links) {
        buffer->mix(link->get_ready_buffer());
    }
}

audio::output::output(const std::string& name_in, node * parent_in)
: audio::channel(name_in, parent_in)
{

}

void audio::output::reset()
{
    std::cout << "creating new output channel buffer" << std::endl;
    buffer = std::make_shared<audio::buffer>();
    buffer->init(parent->get_domain()->buffer_size);
}

void audio::output::connect(audio::input * sink_in)
{
    auto new_link = new audio::link(this, sink_in);
    add_link(new_link);
    sink_in->add_link(new_link);
}

void audio::output::notify()
{
    for(auto link : links) {
        link->notify(buffer);
    }
}

audio::link::link(audio::output * sink_in, audio::input * source_in)
: sink(sink_in), source(source_in)
{

}

std::shared_ptr<audio::buffer> audio::link::get_ready_buffer()
{
    auto lock = make_lock();
    return ready_buffer;
}

audio::link::lock_type audio::link::make_lock()
{
    return lock_type(mutex);
}

void audio::link::reset()
{
    auto lock = make_lock();
    std::cout << "resetting link" << std::endl;
    ready_buffer = nullptr;
    ready_buffer_condition.notify_all();
}

void audio::link::notify(std::shared_ptr<audio::buffer> ready_buffer_in, const bool blocking_in)
{
    auto lock = make_lock();

    if (ready_buffer != nullptr) {
        if (blocking_in) {
            ready_buffer_condition.wait(lock, [this]{ return ready_buffer == nullptr; });
        } else {
            throw std::runtime_error("attempt to set link ready when it was already ready");
        }
    }

    ready_buffer = ready_buffer_in;

    lock.unlock();

    source->link_ready(this);
}

audio::component::component(node * parent_in)
: parent(parent_in)
{

}

audio::component::~component()
{
    for (auto i : sources) {
        delete i.second;
    }

    for(auto i : sinks) {
        delete i.second;
    }

    sources.clear();
    sinks.clear();
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
    for (auto input : sources) {
        input.second->activate();
    }

    for(auto output : sinks) {
        output.second->activate();
    }
}

void audio::component::notify()
{
    for (auto output : sinks) {
        output.second->notify();
    }
}

void audio::component::reset()
{
    pulsar::size_type inputs_with_links = 0;

    for(auto output : sinks) {
        output.second->reset();
    }

    for(auto input : sources) {
        input.second->reset();

        if (input.second->get_links_waiting() > 0) {
            inputs_with_links++;
        }
    }

    sources_waiting.store(inputs_with_links);
}

void audio::component::source_ready(audio::input *)
{
    if (--sources_waiting == 0 && parent->is_ready()) {
        parent->handle_ready();
    }
}

audio::input * audio::component::add_input(const std::string& name_in)
{
    if (sources.count(name_in) != 0) {
        throw std::runtime_error("attempt to add duplicate input name: " + name_in);
    }

    auto new_input = new audio::input(name_in, parent);
    sources[new_input->name] = new_input;
    return new_input;
}

audio::input * audio::component::get_input(const std::string& name_in)
{
    if (sources.count(name_in) == 0) {
        throw std::runtime_error("could not find input channel named " + name_in);
    }

    return sources[name_in];
}

std::vector<std::string> audio::component::get_input_names()
{
    std::vector<std::string> retval;

    for(auto input : sources) {
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
    return new_output;
}

audio::output * audio::component::get_output(const std::string& name_in)
{
    if (sinks.count(name_in) == 0) {
        throw std::runtime_error("could not find output channel named " + name_in);
    }

    return sinks[name_in];
}

std::vector<std::string> audio::component::get_output_names()
{
    std::vector<std::string> retval;

    for(auto output : sinks) {
        retval.push_back(output.first);
    }

    return retval;
}

} // namespace pulsar

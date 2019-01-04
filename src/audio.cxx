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

    pointer = static_cast<pulsar::sample_type *>(std::calloc(size, sizeof(pulsar::sample_type)));

    if (pointer == nullptr) {
        throw std::runtime_error("could not allocate memory for audio buffer");
    }

    own_memory = true;
    size = buffer_size_in;

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

void audio::buffer::set_pointer(pulsar::sample_type * pointer_in)
{
    assert(own_memory == false);
    pointer = pointer_in;
}

void audio::buffer::zero()
{
    assert(pointer != nullptr);
    std::memset(pointer, 0, size);
}

void audio::buffer::mix(buffer * mix_from_in)
{
    if (size != mix_from_in->size) {
        throw std::runtime_error("attempt to mix buffers of different size");
    }

    auto src_p = mix_from_in->get_pointer();

    for(size_type i = 0; i < size; i++) {
        pointer[i] += src_p[i];
    }
}

audio::channel::channel(const std::string &name_in, pulsar::node * parent_in)
: parent(parent_in), name(name_in)
{
    buffer.init(parent->get_domain()->buffer_size);
}

audio::channel::~channel() {

}

void audio::channel::add_link(link * link_in)
{
    links.push_back(link_in);
}

node * audio::channel::get_parent()
{
    return parent;
}

audio::buffer * audio::channel::get_buffer()
{
    return &buffer;
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
        throw std::runtime_error("can't go on CPU yet");
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
        return links[0]->source->get_buffer()->get_pointer();
    } else {
        mix_sinks();
        return buffer.get_pointer();
    }
}

void audio::input::reset()
{
    links_waiting.store(links.size());
}

void audio::input::mix_sinks()
{
    buffer.zero();

    for(auto link : links) {
        buffer.mix(link->source->get_buffer());
    }
}

audio::output::output(const std::string& name_in, node * parent_in)
: audio::channel(name_in, parent_in)
{

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
        link->notify();
    }
}

audio::link::link(audio::output * sink_in, audio::input * source_in)
: sink(sink_in), source(source_in)
{

}

void audio::link::notify()
{
    source->link_ready(this);
}

} // namespace pulsar

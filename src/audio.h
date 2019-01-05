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
#include <vector>

#include "config.h"

namespace pulsar {

struct node;

namespace audio {

struct input;
struct link;
struct output;

class buffer {
    pulsar::size_type size = 0;
    pulsar::sample_type * pointer = nullptr;
    bool own_memory = true;

    public:
    ~buffer();
    void init(const pulsar::size_type buffer_size_in);
    pulsar::size_type get_size();
    pulsar::sample_type * get_pointer();
    void set_pointer(pulsar::sample_type * pointer_in);
    void clear_pointer();
    void release_memory();
    void zero();
    void mix(buffer * mix_from_in);
};

class channel {
    protected:
    node * parent;
    audio::buffer buffer;
    std::vector<link *> links;

    channel(const std::string &name_in, node * parent_in);

    public:
    const std::string name;
    virtual ~channel();
    void activate();
    void add_link(link * link_in);
    node * get_parent();
    audio::buffer * get_buffer();
};

class input : public channel {
    std::atomic<pulsar::size_type> links_waiting = ATOMIC_VAR_INIT(0);

    public:
    input(const std::string& name_in, node * parent_in);
    pulsar::size_type get_links_waiting();
    pulsar::sample_type * get_pointer();
    void connect(output * sink_in);
    void mix_sinks();
    void link_ready(link * link_in);
    void reset();
};

struct output : public channel {
    output(const std::string& name_in, node * parent_in);
    void notify();
    void connect(input * source_in);
};

struct link {
    output * sink;
    input * source;
    link(output * sink_in, input * source_in);
    void notify();
};

class component {
    friend node;

    node * parent = nullptr;
    std::map<std::string, audio::input *> sources;
    std::map<std::string, audio::output *> sinks;
    std::atomic<pulsar::size_type> sources_waiting = ATOMIC_VAR_INIT(0);

    public:
    component(node * parent_in);
    ~component();
    bool is_ready();
    void activate();
    void notify();
    void reset();
    void source_ready(audio::input * ready_source_in);
    pulsar::size_type get_sources_waiting();
    audio::input * add_input(const std::string& name_in);
    audio::input * get_input(const std::string& name_in);
    std::vector<std::string> get_input_names();
    audio::output * add_output(const std::string& name_in);
    audio::output * get_output(const std::string& name_out);
    std::vector<std::string> get_output_names();
};

} // namespace audio

} // namespace pulsar

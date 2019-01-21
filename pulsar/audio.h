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
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <pulsar/node.forward.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace audio {

struct input;
struct input_forward;
struct link;
struct output;
struct output_forward;

class buffer {
    pulsar::size_type size = 0;
    pulsar::sample_type * pointer = nullptr;
    bool own_memory = false;

    public:
    ~buffer();
    void init(const pulsar::size_type buffer_size_in, pulsar::sample_type * pointer_in = nullptr);
    pulsar::size_type get_size();
    pulsar::sample_type * get_pointer();
    void zero();
    void mix(std::shared_ptr<buffer> mix_from_in);
    void set(sample_type * pointer_in, const size_type size_in);
    void set(std::shared_ptr<buffer> buffer_in);
    void scale(const float scale_in);
};

class channel {
    protected:
    node::base * parent;
    std::vector<link *> links;

    channel(const string_type &name_in, node::base * parent_in);

    public:
    const string_type name;
    virtual ~channel();
    virtual void init_cycle() = 0;
    virtual void reset_cycle() = 0;
    void register_link(link * link_in);
    node::base * get_parent();
};

class input : public channel {
    std::atomic<pulsar::size_type> links_waiting = ATOMIC_VAR_INIT(0);
    std::atomic<size_type> num_forwards_to_us = ATOMIC_VAR_INIT(0);
    std::vector<input_forward *> forwards;
    std::map<link *, std::shared_ptr<audio::buffer>> link_buffers;
    mutex_type link_buffers_mutex;

    public:
    virtual void init_cycle();
    virtual void reset_cycle();
    input(const string_type& name_in, node::base * parent_in);
    pulsar::size_type get_links_waiting();
    void link_to(output * to_in);
    void link_to(node::base * node_in, const string_type& port_name_in);
    void forward_to(input * to_in);
    void forward_to(node::base * node_in, const string_type& port_name_in);
    void register_forward(input_forward * forward_in);
    std::shared_ptr<audio::buffer> get_buffer();
    std::shared_ptr<audio::buffer> mix_outputs();
    void link_ready(audio::link * link_in, std::shared_ptr<audio::buffer> buffer_in);
};

class output : public channel {
    std::vector<output_forward *> forwards;
    std::shared_ptr<audio::buffer> buffer;

    public:
    output(const string_type& name_in, node::base * parent_in);
    virtual void init_cycle() override;
    virtual void reset_cycle() override;
    void link_to(input * to_in);
    void link_to(node::base * node_in, const string_type& port_name_in);
    void forward_to(output * to_in);
    void forward_to(node::base * node_in, const string_type& port_name_in);
    void register_forward(output_forward * forward_in);
    std::shared_ptr<audio::buffer> get_buffer();
    void set_buffer(std::shared_ptr<audio::buffer> buffer_in);
    void notify();
};

struct link {
    private:
    mutex_type available_mutex;
    std::condition_variable available_condition;
    std::atomic<bool> available_flag = ATOMIC_VAR_INIT(true);

    public:
    output * sink;
    input * source;
    link(output * sink_in, input * source_in);
    void notify(std::shared_ptr<audio::buffer> ready_buffer_in, const bool blocking_in = true);
    void reset();
};

struct input_forward {
    input * from;
    input * to;
    input_forward(input * from_in, input * to_in);
};

struct output_forward {
    output * from;
    output * to;
    output_forward(output * from_in, output * to_in);
};

class component {
    friend node::base;

    node::base * parent = nullptr;
    std::map<string_type, audio::input *> inputs;
    std::map<string_type, audio::output *> outputs;
    std::atomic<pulsar::size_type> inputs_waiting = ATOMIC_VAR_INIT(0);

    public:
    component(node::base * parent_in);
    ~component();
    bool is_ready();
    void activate();
    void notify();
    void init_cycle();
    void reset_cycle();
    void source_ready(audio::input * ready_source_in);
    pulsar::size_type get_inputs_waiting();
    audio::input * add_input(const string_type& name_in);
    audio::input * get_input(const string_type& name_in);
    std::vector<string_type> get_input_names();
    audio::output * add_output(const string_type& name_in);
    audio::output * get_output(const string_type& name_out);
    std::vector<string_type> get_output_names();
};

} // namespace audio

} // namespace pulsar

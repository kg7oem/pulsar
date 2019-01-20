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

#include "system.h"

namespace pulsar {

namespace node {

namespace base {

struct node;

} // namespace base

} // namespace node

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
    // void set_pointer(pulsar::sample_type * pointer_in);
    // void clear_pointer();
    // void release_memory();
    void zero();
    void mix(std::shared_ptr<buffer> mix_from_in);
    void set(sample_type * pointer_in, const size_type size_in);
    void set(std::shared_ptr<buffer> buffer_in);
    void scale(const float scale_in);
};

class channel {
    protected:
    node::base::node * parent;
    std::vector<link *> links;

    channel(const std::string &name_in, node::base::node * parent_in);

    public:
    const std::string name;
    virtual ~channel();
    void activate();
    virtual void reset() = 0;
    virtual void add_link(link * link_in);
    node::base::node * get_parent();
};

class input : public channel {
    std::atomic<pulsar::size_type> links_waiting = ATOMIC_VAR_INIT(0);
    std::vector<input_forward *> forwards;
    std::map<link *, std::shared_ptr<audio::buffer>> link_buffers;

    public:
    input(const std::string& name_in, node::base::node * parent_in);
    // virtual void add_link(link * link_in) override;
    pulsar::size_type get_links_waiting();
    std::shared_ptr<audio::buffer> get_buffer();
    virtual void reset() override;
    void connect(output * sink_in);
    void forward(input * to_in);
    std::shared_ptr<audio::buffer> mix_sinks();
    void link_ready(link * link_in, std::shared_ptr<audio::buffer> buffer_in);
};

class output : public channel {
    std::vector<output_forward *> forwards;
    std::shared_ptr<audio::buffer> buffer;

    public:
    output(const std::string& name_in, node::base::node * parent_in);
    std::shared_ptr<audio::buffer> get_buffer();
    void set_buffer(std::shared_ptr<audio::buffer> buffer_in, const bool notify_in = false);
    virtual void reset() override;
    void notify(std::shared_ptr<audio::buffer> = nullptr);
    void connect(input * source_in);
    void forward(output * to_in);
};

struct link {
    using mutex_type = std::mutex;
    using lock_type = std::unique_lock<mutex_type>;

    private:
    mutex_type mutex;
    std::condition_variable ready_buffer_condition;
    // TODO remove
    std::shared_ptr<audio::buffer> ready_buffer = nullptr;
    lock_type make_lock();

    public:
    output * sink;
    input * source;
    link(output * sink_in, input * source_in);
    // TODO remove
    std::shared_ptr<audio::buffer> get_ready_buffer();
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
    friend node::base::node;

    node::base::node * parent = nullptr;
    std::map<std::string, audio::input *> sources;
    std::map<std::string, audio::output *> sinks;
    std::atomic<pulsar::size_type> sources_waiting = ATOMIC_VAR_INIT(0);

    public:
    component(node::base::node * parent_in);
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

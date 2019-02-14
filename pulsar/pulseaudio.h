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

#include <pulse/pulseaudio.h>

#include <pulsar/async.h>
#include <pulsar/node.h>
#include <pulsar/types.h>

namespace pulsar {

namespace pulseaudio {

void init();
pa_context * make_context(const string_type& name_in);

enum class node_event {
    pulse_context_ready = 0,
    pulse_input_uncorked
};

struct node : public pulsar::node::base, public std::enable_shared_from_this<node> {
    using notifier_type = async::notifier<node_event>;

    protected:
    notifier_type event;
    pa_context * context = nullptr;
    pa_stream * output_stream = nullptr;
    pa_stream * input_stream = nullptr;
    pa_sample_spec output_spec;
    pa_sample_spec input_spec;
    pa_buffer_attr buffer_attr;
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    void start() override;
    pa_stream * make_stream(const string_type& name_in, const pa_sample_spec * spec_in);
    void uncork(pa_stream * stream_in);

    public:
    virtual void activate() override;
};

class client_node : public node {
    public:
    client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace pulseaudio

} // namespace pulsar

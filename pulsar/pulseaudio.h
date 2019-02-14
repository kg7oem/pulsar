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

struct node : public pulsar::node::base, public std::enable_shared_from_this<node> {
    using state_notifier_type = async::notifier<pa_context_state_t>;

    protected:
    state_notifier_type context_state;
    pa_context * context = nullptr;
    pa_stream * stream = nullptr;
    pa_sample_spec sample_spec;
    pa_buffer_attr buf_attr;
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);

    public:
    virtual void activate() override;
};

class client_node : public node {
    public:
    client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace pulseaudio

} // namespace pulsar

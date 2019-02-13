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

#include <pulsar/node.h>
#include <pulsar/types.h>

namespace pulsar {

namespace pulseaudio {

void init();
pa_context * make_context(const string_type& name_in);

class node : public pulsar::node::base, public std::enable_shared_from_this<node> {
    protected:
    pa_context * context;
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

class client_node : public node {
    public:
    client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace pulseaudio

} // namespace pulsar

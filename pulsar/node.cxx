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
#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include "logging.h"
#include "node.h"

namespace pulsar {

namespace node {

base::node::node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in), name(name_in), audio(this)
{
    assert(domain != nullptr);
}

base::node::~node()
{ }

base::node::lock_type base::node::make_lock()
{
    return lock_type(mutex);
}

std::shared_ptr<domain> base::node::get_domain()
{
    return domain;
}

void base::node::activate()
{
    audio.activate();

    handle_activate();

    reset();
}

void base::node::run()
{
    auto lock = make_lock();
    handle_run();
    reset();
}

void base::node::handle_run()
{
    audio.notify();
}

void base::node::handle_ready()
{
    domain->add_ready_node(this);
}

void base::node::reset()
{
    audio.reset();
}

bool base::node::is_ready()
{
    return audio.is_ready();
}

node::dummy::dummy(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: base::node(name_in, domain_in)
{ }

void node::dummy::handle_activate()
{ }

void node::dummy::handle_run()
{
    auto output_names = audio.get_output_names();
    auto input_names = audio.get_input_names();
    auto num_outputs = output_names.size();

    for(auto&& output_name : output_names) {
        auto output = audio.get_output(output_name);
        auto output_buffer = output->get_buffer();

        output_buffer->zero();

        for(auto&& input_name : input_names) {
            output_buffer->mix(audio.get_input(input_name)->get_buffer());
        }

        output_buffer->scale(1 / num_outputs);
    }

    base::node::handle_run();
}

} // namespace node

} // namespace pulsar

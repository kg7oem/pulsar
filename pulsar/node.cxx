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
#include <iostream>
#include <stdexcept>

#include "node.h"

namespace pulsar {

node::base::base(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in), name(name_in), audio(this)
{
    assert(domain != nullptr);
}

node::base::~base()
{ }

node::base::lock_type node::base::make_lock()
{
    return lock_type(mutex);
}

std::shared_ptr<domain> node::base::get_domain()
{
    return domain;
}

void node::base::activate()
{
    audio.activate();

    handle_activate();

    reset();
}

void node::base::run()
{
    auto lock = make_lock();
    handle_run();
    reset();
}

void node::base::handle_run()
{
    audio.notify();
}

void node::base::handle_ready()
{
    domain->add_ready_node(this);
}

void node::base::reset()
{
    audio.reset();
}

bool node::base::is_ready()
{
    return audio.is_ready();
}

node::dummy::dummy(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: node::base(name_in, domain_in)
{ }

void node::dummy::handle_activate()
{ }

void node::dummy::handle_run()
{
    auto output_names = audio.get_output_names();
    auto input_names = audio.get_input_names();
    auto num_outputs = output_names.size();

    for(auto output_name : output_names) {
        auto output = audio.get_output(output_name);
        auto output_buffer = output->get_buffer();

        output_buffer->zero();

        for(auto input_name : input_names) {
            output_buffer->mix(audio.get_input(input_name)->get_buffer());
        }

        output_buffer->scale(1 / num_outputs);
    }

    node::base::handle_run();
}

} // namespace pulsar

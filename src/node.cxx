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
#include <stdexcept>

#include "node.h"

namespace pulsar {

node::node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in), name(name_in)
{ }

node::~node()
{
    for (auto i : sources) {
        delete i;
    }

    for(auto i : sinks) {
        delete i;
    }

    sources.clear();
    sinks.clear();
}

pulsar::size_type node::get_sources_waiting()
{
    return sources_waiting.load();
}

std::shared_ptr<domain> node::get_domain()
{
    return domain;
}

void node::activate()
{

}

void node::reset()
{
    pulsar::size_type inputs_with_links = 0;

    for(auto&& input : sources) {
        input->reset();
        if (input->get_links_waiting() > 0) {
            inputs_with_links++;
        }
    }

    sources_waiting.store(inputs_with_links);
}

void node::source_ready(audio::input *)
{
    if (--sources_waiting == 0) {
        throw std::runtime_error("can't schedule execution yet");
    }
}

bool node::is_ready()
{
    return get_sources_waiting() == 0;
}

audio::input * node::add_input(const std::string& name_in)
{
    auto new_input = new audio::input(name_in, this);
    sources.push_back(new_input);
    return new_input;
}

audio::input * node::get_input(const std::string& name_in)
{
    for (auto input : sources) {
        if (input->name == name_in) {
            return input;
        }
    }

    throw std::runtime_error("could not find input channel named " + name_in);
}

audio::output * node::add_output(const std::string& name_in)
{
    auto new_output = new audio::output(name_in, this);
    sinks.push_back(new_output);
    return new_output;
}

audio::output * node::get_output(const std::string& name_in)
{
    for (auto output : sinks) {
        if (output->name == name_in) {
            return output;
        }
    }

    throw std::runtime_error("could not find output channel named " + name_in);
}

} // namespace pulsar

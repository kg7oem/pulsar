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

node::node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in), name(name_in), audio(this)
{ }

node::~node()
{ }

std::shared_ptr<domain> node::get_domain()
{
    return domain;
}

void node::activate()
{
    audio.activate();

    handle_activate();
}

// FIXME placeholder for pure virtual function
void node::handle_activate()
{

}

void node::run()
{
    handle_run();

    audio.notify();
}

// FIXME placeholder for pure virtual function
void node::handle_run()
{
    using namespace std::chrono_literals;

    std::cout << "Running node: " << name << std::endl;
    std::this_thread::sleep_for(1s);
}

void node::reset()
{
    audio.reset();
}

bool node::is_ready()
{
    return audio.is_ready();
}

} // namespace pulsar

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
#include <memory>
#include <string>
#include <vector>

#include "audio.h"
#include "config.h"
#include "domain.h"

namespace pulsar {

class node {
    std::shared_ptr<pulsar::domain> domain;
    std::vector<audio::input *> sources;
    std::vector<audio::output *> sinks;
    std::atomic<pulsar::size_type> sources_waiting = ATOMIC_VAR_INIT(0);

    public:
    const std::string name;
    node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual ~node();
    std::shared_ptr<pulsar::domain> get_domain();
    void activate();
    void reset();
    void source_ready(audio::input * ready_source_in);
    pulsar::size_type get_sources_waiting();
    bool is_ready();
    audio::input * add_input(const std::string& name_in);
    audio::input * get_input(const std::string& name_in);
    audio::output * add_output(const std::string& name_in);
    audio::output * get_output(const std::string& name_out);
}; // struct node

} // namespace pulsar

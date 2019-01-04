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
#include <map>
#include <memory>
#include <string>

#include "audio.h"
#include "config.h"
#include "domain.h"

namespace pulsar {

class node {
    std::shared_ptr<pulsar::domain> domain;

    protected:
    // FIXME should be pure virtual
    virtual void handle_activate();
    virtual void handle_run();

    public:
    const std::string name;
    audio::component audio;
    node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual ~node();
    std::shared_ptr<pulsar::domain> get_domain();
    void activate();
    void run();
    void reset();
    bool is_ready();
}; // struct node

} // namespace pulsar

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

#include <pulsar/async.h>
#include <pulsar/audio.h>
#include <pulsar/library.h>
#include <pulsar/node.h>

namespace pulsar {

namespace zeronode {

void init();
pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in);

class node : public pulsar::node::io {
    protected:
    std::shared_ptr<async::timer> timer = nullptr;
    mutex_type busy_mutex;
    bool busy_flag = false;
    void start() override;
    void execute() override;
    void handle_timer();
    virtual void input_ready();
    virtual void stop() override;

    public:
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    virtual void activate() override;
};

} // namespace zeronode

} // namespace pulsar

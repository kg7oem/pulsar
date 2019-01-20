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

#include "async.h"
#include "audio.h"
#include "library.h"
#include "node.h"

namespace pulsar {

namespace jackaudio {

extern "C" {
#include <jack/jack.h>
}

using client_type = jack_client_t;
using flags_type = unsigned long;
using options_type = jack_options_t;
using port_type = jack_port_t;
using size_type = unsigned long;

void init();
pulsar::node::base::node * make_node(const std::string& name_in, std::shared_ptr<domain> domain_in);

class node : public pulsar::node::base::node {
    std::shared_ptr<async::watchdog> watchdog = nullptr;
    client_type * jack_client = nullptr;
    const options_type jack_options = JackNoStartServer;
    std::map<std::string, port_type *> jack_ports;
    std::atomic<bool> did_notify = ATOMIC_VAR_INIT(false);
    std::condition_variable done_cond;
    mutex_type done_mutex;
    bool done_flag = false;
    lock_type make_done_lock();
    port_type * add_port(const std::string& port_name_in, const char * port_type_in, const flags_type flags_in, const size_type buffer_size_in = 0);
    sample_type * get_port_buffer(const std::string& port_name_in);
    void start();
    void open(const std::string& jack_name_in);
    virtual void run() override;
    virtual void notify() override;
    void handle_jack_process(jack_nframes_t nframes_in);

    public:
    node(const std::string& name_in, std::shared_ptr<pulsar::domain> domain_in);
    ~node();
    virtual void activate() override;
};

} // namespace jackaudio

} // namespace pulsar

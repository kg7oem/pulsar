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

namespace jackaudio {

extern "C" {
#include <jack/jack.h>
}

using client_type = jack_client_t;
using flags_type = unsigned long;
using options_type = jack_options_t;
using port_type = jack_port_t;
using size_type = unsigned long;

class node : public pulsar::node::base {
    std::shared_ptr<async::watchdog> watchdog = nullptr;
    client_type * jack_client = nullptr;
    const options_type jack_options = JackNoStartServer;
    std::map<string_type, port_type *> jack_ports;
    std::atomic<bool> did_notify = ATOMIC_VAR_INIT(false);
    std::condition_variable done_cond;
    mutex_type done_mutex;
    bool done_flag = false;
    port_type * add_port(const string_type& port_name_in, const char * port_type_in, const flags_type flags_in, const size_type buffer_size_in = 0);
    sample_type * get_port_buffer(const string_type& port_name_in);
    void open(const string_type& jack_name_in);
    void handle_jack_shutdown(jack_status_t status_in, const char * message_in);
    void handle_jack_process(jack_nframes_t nframes_in);

    /* lifecycle methods */
    void start() override;
    virtual void run() override;
    virtual void notify() override;
    virtual void execute() override;
    virtual void stop() override;

    public:
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    ~node();
    virtual void activate() override;
};

class connections : public daemon::base {
    using list_type = std::list<std::pair<string_type, string_type>>;

    mutex_type jack_mutex;
    string_type client_name = "";
    client_type * jack_client = nullptr;
    const options_type jack_options = JackNoStartServer;
    list_type connection_list;
    void register_callbacks();

    public:
    connections(const string_type& name_in);
    virtual ~connections();
    virtual void init(const YAML::Node& yaml_in) override;
    virtual void start() override;
    virtual void stop() override;
    void check_port_connections(const string_type& name_in);
    std::map<string_type, bool> get_connection_lookup(const string_type& port_name_in);
};

void init();
pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in);
std::shared_ptr<connections> make_connections(const string_type& name_in);

} // namespace jackaudio

} // namespace pulsar

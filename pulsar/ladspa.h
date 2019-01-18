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

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "node.h"

namespace pulsar {

namespace ladspa {

extern "C" {
#include <ladspa.h>
}

using data_type = LADSPA_Data;
using descriptor_type = LADSPA_Descriptor;
using handle_type = LADSPA_Handle;
using id_type = size_type;
using port_descriptor_type = LADSPA_PortDescriptor;

struct file;
struct instance;

std::shared_ptr<file> open(const std::string& path_in);
handle_type open(const std::string& path_in, const id_type id_in, const size_type sample_rate_in);
const ladspa::descriptor_type * open(const std::string& path_in, const id_type id_in);
ladspa::handle_type open(const std::string& path_in, const id_type id_in, const pulsar::size_type sample_rate_in);
std::shared_ptr<instance> make_instance(std::shared_ptr<file> file_in, const id_type id_in, const pulsar::size_type sample_rate_in);
std::shared_ptr<instance> make_instance(const std::string& path_in, const id_type id_in, const pulsar::size_type sample_rate_in);
data_type get_control_port_default(const descriptor_type * descriptor_in, const size_type port_num_in);

class file : public std::enable_shared_from_this<file> {
    void *handle = nullptr;
    LADSPA_Descriptor_Function descriptor_fn = nullptr;
    std::map<id_type, const descriptor_type *> id_to_descriptor;

    public:
    const std::string path;
    const std::vector<const descriptor_type *> get_descriptors();
    const descriptor_type * get_descriptor(const id_type id_in);
    const std::vector<std::pair<id_type, std::string>> enumerate();
    file(const std::string &path_in);
    virtual ~file();
    std::shared_ptr<instance> make_instance(const id_type id_in, const pulsar::size_type sample_rate_in);
};

class instance : public std::enable_shared_from_this<instance> {
    std::shared_ptr<ladspa::file> file = nullptr;
    const descriptor_type * descriptor = nullptr;
    handle_type handle = nullptr;
    std::map<std::string, size_type> port_name_to_num;

    public:
    instance(std::shared_ptr<ladspa::file> file_in, const descriptor_type * descriptor_in, const pulsar::size_type sample_rate_in);
    const descriptor_type * get_descriptor();
    size_type get_port_count();
    port_descriptor_type get_port_descriptor(const size_type port_num_in);
    const std::string get_port_name(const size_type port_num_in);
    const std::vector<std::string> get_port_names();
    id_type get_port_num(const std::string &name_in);
    void activate();
    void connect(const size_type port_num_in, data_type * buffer_in);
    void run(const size_type num_samples_in);
};

class node : public pulsar::node::base::node {
    void setup();

    protected:
    std::shared_ptr<ladspa::instance> ladspa = nullptr;
    virtual void handle_activate() override;
    virtual void handle_run() override;

    public:
    node(const std::string& name_in, std::shared_ptr<ladspa::instance> instance_in, std::shared_ptr<pulsar::domain> domain_in);
    node(const std::string& name_in, const std::string& path_in, const id_type id_in, std::shared_ptr<pulsar::domain> domain_in);
};

} // namespace ladspa

} // namespace pulsar

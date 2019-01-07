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
#include <dlfcn.h>

#include "ladspa.h"

#define DESCRIPTOR_SYMBOL "ladspa_descriptor"

namespace pulsar {

namespace ladspa {

static std::map<std::string, std::shared_ptr<file>> loaded_files;

std::shared_ptr<file> open(const std::string& path_in)
{
    auto result = loaded_files.find(path_in);

    if (result != loaded_files.end()) {
        return result->second;
    }

    auto new_file = std::make_shared<file>(path_in);
    loaded_files[path_in] = new_file;

    return new_file;
}

const ladspa::descriptor_type * open(const std::string& path_in, const id_type id_in)
{
    auto file = open(path_in);
    return file->get_descriptor(id_in);
}

ladspa::handle_type open(const std::string& path_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto descriptor = open(path_in, id_in);
    auto instance = descriptor->instantiate(descriptor, sample_rate_in);

    if (instance == nullptr) {
        throw std::runtime_error("could not instantiate ladspa plugin");
    }

    return instance;
}

std::shared_ptr<instance> make_instance(std::shared_ptr<file> file_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto descriptor = file_in->get_descriptor(id_in);
    return std::make_shared<instance>(file_in, descriptor, sample_rate_in);
}

std::shared_ptr<instance> make_instance(const std::string& path_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto file = open(path_in);
    return make_instance(file, id_in, sample_rate_in);
}

file::file(const std::string &path_in)
: path(path_in)
{
    handle = dlopen(path_in.c_str(), RTLD_NOW);
    if (handle == nullptr) {
        throw std::runtime_error("could not dlopen(" + path + ")");
    }

    descriptor_fn = (LADSPA_Descriptor_Function) dlsym(handle, DESCRIPTOR_SYMBOL);
    if (descriptor_fn == nullptr) {
        throw std::runtime_error("could not get descriptor function for " + path);
    }

    for (auto i : get_descriptors()) {
        id_to_descriptor[i->UniqueID] = i;
    }
}

file::~file()
{
    if (handle != nullptr) {
        if (dlclose(handle)) {
            throw std::runtime_error("could not dlclose() ladspa plugin handle");
        }

        handle = nullptr;
        descriptor_fn = nullptr;
    }
}

std::shared_ptr<instance> file::make_instance(const id_type id_in, const pulsar::size_type sample_rate_in)
{
    return ladspa::make_instance(this->shared_from_this(), id_in, sample_rate_in);
}

const descriptor_type * file::get_descriptor(const id_type id_in)
{
    auto result = id_to_descriptor.find(id_in);

    if (result != id_to_descriptor.end()) {
        return result->second;
    }

    throw std::runtime_error("could not find type id in plugin");
}

const std::vector<const descriptor_type *> file::get_descriptors()
{
    std::vector<const descriptor_type *> descriptors;
    const LADSPA_Descriptor * p;

    for(long i = 0; (p = descriptor_fn(i)) != nullptr; i++) {
        descriptors.push_back(p);
    }

    return descriptors;
}

const std::vector<std::pair<id_type, std::string>> file::enumerate()
{
    std::vector<std::pair<id_type, std::string>> retval;

    for (auto descriptor : get_descriptors()) {
        retval.emplace_back(descriptor->UniqueID, descriptor->Name);
    }

    return retval;
}

instance::instance(std::shared_ptr<ladspa::file> file_in, const descriptor_type * descriptor_in, const pulsar::size_type sample_rate_in)
: file(file_in), descriptor(descriptor_in)
{
    assert(file_in != nullptr);
    assert(descriptor_in != nullptr);

    handle = descriptor->instantiate(descriptor, sample_rate_in);

    if (handle == nullptr) {
        throw std::runtime_error("could not instantiate LADSPA");
    }
}

void instance::activate()
{
    control_buffers.reserve(descriptor->PortCount);

    if (descriptor->activate != nullptr) {
        descriptor->activate(handle);
    }
}

node::node(const std::string& name_in, std::shared_ptr<ladspa::instance> instance_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in), ladspa(instance_in)
{
    setup();
}

node::node(const std::string& name_in, const std::string& path_in, const id_type id_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in), ladspa(make_instance(path_in, id_in, domain_in->sample_rate))
{
    setup();
}

void node::setup()
{
    assert(domain != nullptr);
    assert(ladspa != nullptr);

    // TODO
    // add audio input and output channels from LADSPA ports
}

void node::handle_activate()
{
    ladspa->activate();
}

} // namespace ladspa

} // namespace pulsar

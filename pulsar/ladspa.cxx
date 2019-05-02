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
#include <cmath>
#include <dlfcn.h>

#include <pulsar/ladspa.h>
#include <pulsar/logging.h>
#include <pulsar/system.h>

#define DESCRIPTOR_SYMBOL "ladspa_descriptor"

namespace pulsar {

namespace ladspa {

static std::map<string_type, std::shared_ptr<file>> loaded_files;

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<ladspa::node>(name_in);
}

void init()
{
    library::register_node_factory("pulsar::ladspa::node", make_node);
}

std::shared_ptr<file> open(const string_type& path_in)
{
    auto result = loaded_files.find(path_in);

    if (result != loaded_files.end()) {
        return result->second;
    }

    auto new_file = std::make_shared<file>(path_in);
    loaded_files[path_in] = new_file;

    return new_file;
}

const ladspa::descriptor_type * open(const string_type& path_in, const id_type id_in)
{
    auto file = open(path_in);
    return file->get_descriptor(id_in);
}

ladspa::handle_type open(const string_type& path_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto descriptor = open(path_in, id_in);
    auto instance = descriptor->instantiate(descriptor, sample_rate_in);

    if (instance == nullptr) {
        system_fault("could not instantiate ladspa plugin");
    }

    return instance;
}

std::shared_ptr<instance> make_instance(std::shared_ptr<file> file_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto descriptor = file_in->get_descriptor(id_in);
    return std::make_shared<instance>(file_in, descriptor, sample_rate_in);
}

std::shared_ptr<instance> make_instance(const string_type& path_in, const id_type id_in, const pulsar::size_type sample_rate_in)
{
    auto file = open(path_in);
    return make_instance(file, id_in, sample_rate_in);
}

data_type get_control_port_default(const descriptor_type * descriptor_in, const size_type port_num_in)
{
    auto port_hints = descriptor_in->PortRangeHints[port_num_in];
    auto hint_descriptor = port_hints.HintDescriptor;

    if (! LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {
        return 0;
    } else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
        return 0;
    } else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {
        return 1;
    } else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {
        return 100;
    } else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
        return 440;
    } else if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
        return port_hints.LowerBound;
    } else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.75 + log(port_hints.UpperBound) * 0.25);
        } else {
            return port_hints.LowerBound * 0.75 + port_hints.UpperBound * 0.25;
        }
    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.5 + log(port_hints.UpperBound) * 0.5);
        } else {
            return (port_hints.LowerBound * 0.5 + port_hints.UpperBound * 0.5);
        }
    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.25 + log(port_hints.UpperBound) * 0.75);
        } else {
            return port_hints.LowerBound * 0.25 + port_hints.UpperBound * 0.75;
        }
    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
        return port_hints.UpperBound;
    }

    throw std::logic_error("could not find hint for LADSPA port");
}

file::file(const string_type &path_in)
: path(path_in)
{
    handle = dlopen(path_in.c_str(), RTLD_NOW);
    if (handle == nullptr) {
        system_fault("could not dlopen(" + path + ")");
    }

    descriptor_fn = (LADSPA_Descriptor_Function) dlsym(handle, DESCRIPTOR_SYMBOL);
    if (descriptor_fn == nullptr) {
        system_fault("could not get descriptor function for " + path);
    }

    for (auto&& i : get_descriptors()) {
        id_to_descriptor[i->UniqueID] = i;
    }
}

file::~file()
{
    if (handle != nullptr) {
        if (dlclose(handle)) {
            system_fault("could not dlclose() ladspa plugin handle");
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
    auto id = id_in;

    // FIXME does LADSPA say that id 0 will never be used?
    if (id == 0) {
        auto type_list = enumerate();

        if (type_list.size() != 1) {
            system_fault("no LADSPA type id was specified and there was more than one type in the file");
        }

        id = type_list[0].first;
    }

    auto result = id_to_descriptor.find(id);

    if (result != id_to_descriptor.end()) {
        return result->second;
    }

    system_fault("could not find type id in plugin");
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

const std::vector<std::pair<id_type, string_type>> file::enumerate()
{
    std::vector<std::pair<id_type, string_type>> retval;

    for (auto&& descriptor : get_descriptors()) {
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
        system_fault("could not instantiate LADSPA");
    }

    for(size_type i = 0; i < get_port_count(); i++) {
        string_type name(descriptor->PortNames[i]);
        port_name_to_num[name] = i;
    }
}

const descriptor_type * instance::get_descriptor()
{
    return descriptor;
}

size_type instance::get_port_count()
{
    return descriptor->PortCount;
}

port_descriptor_type instance::get_port_descriptor(const size_type port_num_in)
{
    return descriptor->PortDescriptors[port_num_in];
}

const string_type instance::get_port_name(const size_type port_num_in)
{
    return string_type(descriptor->PortNames[port_num_in]);
}

id_type instance::get_port_num(const string_type& name_in)
{
    auto result = port_name_to_num.find(name_in);

    if (result == port_name_to_num.end()) {
        system_fault("could not find port with name " + name_in);
    }

    return result->second;
}

void instance::activate()
{
    if (descriptor->activate != nullptr) {
        descriptor->activate(handle);
    }
}

void instance::connect(const size_type port_num_in, data_type * buffer_in)
{
    descriptor->connect_port(handle, port_num_in, buffer_in);
}

void instance::run(const size_type num_samples_in)
{
    descriptor->run(handle, num_samples_in);
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::filter(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).value->set("pulsar::ladspa::node");
    add_property("plugin:filename", property::value_type::string);
    add_property("plugin:id", property::value_type::size);
    add_property("plugin:label", property::value_type::string);
}

void node::init()
{
    assert(domain != nullptr);
    assert(ladspa == nullptr);

    auto ladspa_file = get_property("plugin:filename").value->get_string();
    auto ladspa_id = get_property("plugin:id").value->get_size();

    ladspa = make_instance(ladspa_file, ladspa_id, domain->sample_rate);

    get_property("plugin:label").value->set(ladspa->get_descriptor()->Label);
    get_property("plugin:id").value->set(ladspa->get_descriptor()->UniqueID);

    auto port_count = ladspa->get_port_count();

    for(size_type port_num = 0; port_num < port_count; port_num++) {
        auto port_name = ladspa->get_port_name(port_num);
        auto descriptor = ladspa->get_port_descriptor(port_num);

        if (LADSPA_IS_PORT_AUDIO(descriptor)) {
            ladspa->connect(port_num, nullptr);

            if (LADSPA_IS_PORT_INPUT(descriptor)) {
                audio.add_input(port_name);
            } else if (LADSPA_IS_PORT_OUTPUT(descriptor)) {
                audio.add_output(port_name);
            } else {
                system_fault("LADSPA port was neither input nor output");
            }
        } else if (LADSPA_IS_PORT_CONTROL(descriptor)) {
            auto default_value = get_control_port_default(ladspa->get_descriptor(), port_num);
            string_type property_name;

            if (LADSPA_IS_PORT_OUTPUT(descriptor)) {
                property_name = "state:";
            } else if (LADSPA_IS_PORT_INPUT(descriptor)) {
                property_name = "config:";
            } else {
                system_fault("LADSPA port was neither input nor output");
            }

            property_name += port_name;

            auto& control = add_property(property_name, property::value_type::real);
            control.value->set(default_value);

            ladspa->connect(port_num, &control.value->get_real());
        } else {
            system_fault("LADSPA port was neither audio nor control");
        }
    }

    pulsar::node::filter::init();
}

void node::activate()
{
    ladspa->activate();
    pulsar::node::filter::activate();
}

void node::run()
{
    log_trace("running LADSPA plugin for node ", name);

    for (auto&& port_name : audio.get_input_names()) {
        auto buffer = audio.get_input(port_name)->get_buffer();
        auto port_num = ladspa->get_port_num(port_name);
        ladspa->connect(port_num, buffer->get_pointer());
    }

    for (auto&& port_name : audio.get_output_names()) {
        auto buffer = audio.get_output(port_name)->get_buffer();
        auto port_num = ladspa->get_port_num(port_name);
        ladspa->connect(port_num, buffer->get_pointer());
    }

    ladspa->run(domain->buffer_size);

    for (auto&& port_name : audio.get_input_names()) {
        auto port_num = ladspa->get_port_num(port_name);
        ladspa->connect(port_num, nullptr);
    }

    for (auto&& port_name : audio.get_output_names()) {
        auto port_num = ladspa->get_port_num(port_name);
        ladspa->connect(port_num, nullptr);
    }

    log_trace("done running LADSPA plugin for node ", name);
}

} // namespace ladspa

} // namespace pulsar

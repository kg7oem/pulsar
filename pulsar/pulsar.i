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

%module pulsar

// shared ptr library does not work with perl
// %include "shared_ptr.i"
%include "std_map.i"
%include "std_string.i"
%include "std_vector.i"

%{

#include <pulsar/pulsar.h>

%}

// %shared_ptr(pulsar::domain)

#include <pulsar/forward.h>

struct pulsar::domain {
    const std::string name;
    const unsigned long sample_rate;
    const unsigned long buffer_size;
    domain(const std::string& name_in, const unsigned long sample_rate_in, const unsigned long buffer_size_in);
    virtual ~domain();
    void activate(const unsigned long num_threads_in = 1);
};

%rename(bootstrap) pulsar_bootstrap;
void pulsar_bootstrap();

%rename(make_domain) pulsar_make_domain;
std::shared_ptr<pulsar::domain> pulsar_make_domain(const std::string& name_in, const unsigned long sample_rate_in, const unsigned long buffer_size_in);

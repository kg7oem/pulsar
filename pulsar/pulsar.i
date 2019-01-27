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

%{

#include "pulsar/pulsar.h"

%}

namespace pulsar {

namespace system {

void bootstrap();

} // namespace system

class domain {
    const string_type name;

    domain(const string_type& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in);
    virtual ~domain();
    void activate(const size_type num_threads_in = 1);
};

} // namespace pulsar

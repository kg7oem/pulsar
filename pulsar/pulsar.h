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

extern "C" {

typedef unsigned long pulsar_size_type;

struct pulsar_domain {
    void * ptr;
};

void pulsar_bootstrap();

pulsar_domain * pulsar_create_domain(const char * name_in, const pulsar_size_type sample_rate_in, const pulsar_size_type buffer_size_in);
void pulsar_destroy_domain(pulsar_domain * domain_in);

} // extern "C"
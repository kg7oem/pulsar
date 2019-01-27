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
#include <cstdlib>

#include <pulsar/pulsar.h>

struct pulsar_domain
{
    std::shared_ptr<pulsar::domain> ptr;

    pulsar_domain(const char * name_in, const unsigned long sample_rate_in, const unsigned long buffer_size_in);
};

extern "C" {

void pulsar_bootstrap()
{
    pulsar::system::bootstrap();
}

pulsar_domain * pulsar_create_domain(const char * name_in, const unsigned long sample_rate_in, const unsigned long buffer_size_in)
{
    return new pulsar_domain(name_in, sample_rate_in, buffer_size_in);
}

void pulsar_destroy_domain(pulsar_domain * domain_in)
{
    delete domain_in;
}

const char * pulsar_domain_get_name(pulsar_domain * domain_in)
{
    return domain_in->ptr->name.c_str();
}

} // extern "C"

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

#include <pulsar/domain.h>
#include <pulsar/pulsar.h>

extern "C" {

void pulsar_bootstrap()
{
    pulsar::system::bootstrap();
}

pulsar_domain * pulsar_create_domain(const char * name_in, const pulsar_size_type sample_rate_in, const pulsar_size_type buffer_size_in)
{
    auto new_domain = new pulsar::domain(name_in, sample_rate_in, buffer_size_in);
    auto new_shared_ptr = new std::shared_ptr<pulsar::domain>(new_domain);
    struct pulsar_domain * new_wrapper = static_cast<pulsar_domain *>(std::malloc(sizeof(struct pulsar_domain)));

    if (new_wrapper == nullptr) {
        system_fault("could not allocate memory for wrapper");
    }

    new_wrapper->ptr = static_cast<void *>(new_domain);
}

void pulsar_destroy_domain(pulsar_domain * domain_in)
{
    assert(domain_in != nullptr);
    assert(domain_in->ptr != nullptr);

    delete static_cast<std::shared_ptr<pulsar::domain> *>(domain_in->ptr);
    free(domain_in);
}

} // extern "C"

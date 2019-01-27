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

#if __cplusplus
#include <pulsar/domain.h>

extern "C" {
#endif

void pulsar_bootstrap();

struct pulsar_domain;
pulsar_domain * pulsar_create_domain(const char * name_in, const unsigned long sample_rate_in, const unsigned long buffer_size_in);
void pulsar_destroy_domain(pulsar_domain * domain_in);
const char * pulsar_domain_get_name(pulsar_domain * domain_in);

#if __cplusplus
} // extern "C"
#endif

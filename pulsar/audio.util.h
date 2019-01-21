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

#include <pulsar/system.h>

namespace pulsar {

namespace audio {

namespace util {
    void pcm_zero(sample_type * dest_in, const size_type samples_in);
    void pcm_scale(sample_type * dest_in, const float scale_in, const size_type samples_in);
    void pcm_set(sample_type * dest_in, const sample_type * src_in, const size_type samples_in);
    void pcm_mix(sample_type * dest_in, const sample_type * src_in, const size_type samples_in);
}

} // namespace audio

} // namespace pulsar

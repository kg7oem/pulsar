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
#include <cstring>
#include <iostream>

#include "audio.util.h"

namespace pulsar {

void audio::util::pcm_zero(sample_type * dest_in, const size_type samples_in)
{
    assert(dest_in != nullptr);
    std::memset(dest_in, 0, samples_in * sizeof(sample_type));
}

void audio::util::pcm_scale(sample_type * dest_in, const float scale_in, const size_type samples_in)
{
    assert(dest_in != nullptr);

    for(size_type i = 0; i < samples_in; i++) {
        dest_in[i] *= scale_in;
    }
}

void audio::util::pcm_set(sample_type * dest_in, const sample_type * src_in, const size_type samples_in)
{
    assert(dest_in != nullptr);
    assert(src_in != nullptr);
    // std::cout << "audio::util::pcm_set(): ";
    // for(size_type i = 0; i < samples_in; i++) {
    //     std::cout << src_in[i] << " ";
    // }
    // std::cout << std::endl;
    memcpy(dest_in, src_in, samples_in * sizeof(sample_type));
}

void audio::util::pcm_mix(sample_type * dest_in, const sample_type * src_in, const size_type samples_in)
{
    assert(dest_in != nullptr);
    assert(src_in != nullptr);

    for(size_type i = 0; i < samples_in; i++) {
        dest_in[i] += src_in[i];
    }
}

} // namespace pulsar

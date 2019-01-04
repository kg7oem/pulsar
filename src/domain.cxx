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

#include "domain.h"

namespace pulsar {

domain::domain(const std::string& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in)
: name(name_in), sample_rate(sample_rate_in), buffer_size(buffer_size_in)
{
    zero_buffer.init(buffer_size_in);
}

domain::~domain()
{ }

audio::buffer& domain::get_zero_buffer()
{
    return zero_buffer;
}

} // namespace pulsar

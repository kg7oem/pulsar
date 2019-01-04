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
#include <string>
#include <vector>

#include "audio.h"
#include "config.h"

namespace pulsar {

class node;

class domain : public std::enable_shared_from_this<domain> {
    audio::buffer zero_buffer;

    public:
    const std::string name;
    const pulsar::size_type sample_rate;
    const pulsar::size_type buffer_size;
    domain(const std::string& name_in, const pulsar::size_type sample_rate_in, const pulsar::size_type buffer_size_in);
    virtual ~domain();
    audio::buffer& get_zero_buffer();

    template<class T, typename... Args>
    std::shared_ptr<node> make_node(Args... args)
    {
        return std::make_shared<T>(args..., this->shared_from_this());
    }
};

} // namespace pulsar

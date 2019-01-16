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

#include <boost/version.hpp>
#include <stdexcept>
#include <string>

#include "system.h"

namespace pulsar {

namespace system {

std::string make_boost_version();

const std::string boost_version = make_boost_version();

const std::string& get_boost_version()
{
    return boost_version;
}

std::string make_boost_version()
{
    std::string buf("v");
    std::string version(BOOST_LIB_VERSION);

    auto pos = version.find("_");

    if (pos == std::string::npos) {
        throw std::runtime_error("expected to find _ in " + version);
    }

    version.replace(pos, 1, ".");

    pos = version.find("_");

    if (pos == std::string::npos) {
        version += ".0";
    } else {
        version.replace(pos, 1, ".");
    }

    buf += version;

    return buf;
}

} // namespace system

} // namespace pulsar

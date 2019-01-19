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

#include "util.h"

namespace pulsar {

namespace util {

namespace string {

// from https://stackoverflow.com/a/236803
std::vector<std::string> split(const std::string& string_in, const char delim_in)
{
    std::vector<std::string> parts;
    split(string_in, delim_in, std::back_inserter(parts));
    return parts;
}

} // namespace string

} // namespace util

} //namespace pulsar


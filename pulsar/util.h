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

#include <sstream>
#include <string>
#include <vector>

namespace pulsar {

namespace util {

namespace string {

// from https://stackoverflow.com/a/236803
template<typename Out>
void split(const std::string &string_in, const char delim_in, Out result) {
    std::stringstream ss(string_in);
    std::string item;
    while (std::getline(ss, item, delim_in)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string& string_in, const char delim_in);

} // namespace string

} // namespace util

} //namespace pulsar

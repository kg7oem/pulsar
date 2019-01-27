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

#include <pulsar/types.h>

namespace pulsar {

namespace util {

// from https://stackoverflow.com/a/236803
template<typename Out>
void split(const string_type &string_in, const char delim_in, Out result) {
    std::stringstream ss(string_in);
    string_type item;
    while (std::getline(ss, item, delim_in)) {
        *(result++) = item;
    }
}

std::vector<string_type> split(const string_type& string_in, const char delim_in);

template <typename T>
void sstream_accumulate_vaargs(std::stringstream& sstream, T&& t) {
    sstream << t;
}

template <typename T, typename... Args>
void sstream_accumulate_vaargs(std::stringstream& sstream, T&& t, Args&&... args) {
    sstream_accumulate_vaargs(sstream, t);
    sstream_accumulate_vaargs(sstream, args...);
}

template <typename... Args>
std::string to_string(Args&&... args) {
    std::stringstream buf;
    sstream_accumulate_vaargs(buf, args...);
    return buf.str();
}

} // namespace util

} //namespace pulsar

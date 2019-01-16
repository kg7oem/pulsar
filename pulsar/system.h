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

#include <exception>
#include <sstream>
#include <stdexcept>

// g++ 6.3.0 as it comes in debian/stretch does not support maybe_unused
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED [[ maybe_unused ]]
#endif

namespace oemros {

#define system_fault(...) oemros::system_fault__func(__FILE__, __LINE__, __PRETTY_FUNCTION__, oemros::vaargs_to_string(__VA_ARGS__))
#define system_panic(...) oemros::system_panic__func(__FILE__, __LINE__, __PRETTY_FUNCTION__, oemros::vaargs_to_string(__VA_ARGS__))

enum class exit_code {
    ok = 0,
    fault = 1,
    doublefault = 2,
    panic = 3,
};

exit_code get_fault_state();
[[noreturn]] void exit_fault_state();

template <typename T>
void sstream_accumulate_vaargs(std::stringstream& sstream, T t) {
    sstream << t;
}

template <typename T, typename... Args>
void sstream_accumulate_vaargs(std::stringstream& sstream, T t, Args... args) {
    sstream_accumulate_vaargs(sstream, t);
    sstream_accumulate_vaargs(sstream, args...);
}

template <typename... Args>
std::string vaargs_to_string(Args... args) {
    std::stringstream buf;
    sstream_accumulate_vaargs(buf, args...);
    return buf.str();
}

[[noreturn]] void system_fault__func(const char* file_in, int line_in, const char* function_in, const std::string& message_in);
[[noreturn]] void system_panic__func(const char* file_in, int line_in, const char* function_in, const std::string& message_in);

const char* enum_to_str(const exit_code& code_in);

struct exception : public std::exception {
    const std::string message;
    exception(const std::string& message_in);
    virtual const char* what() const noexcept override;
};

struct fault : public exception {
    const char* file;
    int line;
    const char* function;

    fault(const char* file_in, int line_in, const char* function_in, const std::string& message_in);
};

struct double_fault : public fault {
    double_fault(const char* file_in, int line_in, const char* function_in, const char* message_in)
    : fault(file_in, line_in, function_in, message_in) { }
};

std::string errno_str(int errno_in);

}

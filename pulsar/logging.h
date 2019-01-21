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

#include "ext/logjam.h"

#define PULSAR_LOG_NAME "pulsar"

#define log_error(...)   logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::error, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)    logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::info, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_verbose(...) logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::verbose, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...)   logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::debug, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_trace(...)   logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::trace, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_unknown(...) logjam::send_logevent(PULSAR_LOG_NAME, logjam::loglevel::unknown, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

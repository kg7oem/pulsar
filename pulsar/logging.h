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

#define LOGJAM_LOG_MACROS
#undef LOGJAM_NLOG

#include "ext/logjam.h"

#include <pulsar/util.h>

#define PULSAR_LOG_NAME "pulsar"

#define log_error(...)    LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::error, __VA_ARGS__)
#define log_info(...)     LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::info, __VA_ARGS__)
#define log_verbose(...)  LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::verbose, __VA_ARGS__)
#define log_debug(...)    LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::debug, __VA_ARGS__)
#define log_trace(...)    LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::trace, __VA_ARGS__)
#define log_unknown(...)  LOGJAM_LOG_VARGS(PULSAR_LOG_NAME, logjam::loglevel::unknown, __VA_ARGS__)

#define llog_error(block)   LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::error, block)
#define llog_info(block)    LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::info, block)
#define llog_verbose(block) LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::verbose, block)
#define llog_debug(block)   LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::debug, block)
#define llog_trace(block)   LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::trace, block)
#define llog_unknown(block) LOGJAM_LOG_LAMBDA(PULSAR_LOG_NAME, logjam::loglevel::unknown, block)

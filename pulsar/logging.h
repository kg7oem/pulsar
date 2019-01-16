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

#define LOGJAM_LOGSOURCE_MACRO

#include "logjam.h"
#include "system.h"

#define log_error(...)   logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::error, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)    logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::info, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_verbose(...) logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::verbose, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...)   logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::debug, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_trace(...)   logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::trace, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define log_unknown(...) logjam::send_logevent(oemros::log_sources.oemros, logjam::loglevel::unknown, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

namespace oemros {

// TODO this is weird too - is there a way to clean this up?
struct _log_sources {
    LOGJAM_LOGSOURCE(oemros);
    LOGJAM_LOGSOURCE(hamlib);
};

extern const _log_sources log_sources;

class log_engine : public logjam::logengine {
    public:
        log_engine();
};

class log_console : public logjam::logconsole {
    public:
        log_console(const logjam::loglevel& level_in);
};

}

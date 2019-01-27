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

#include <dbus-cxx.h>

#include <pulsar/system.h>
#include <pulsar/thread.h>

#define PULSAR_DBUS_NAME "audio.pulsar"

namespace pulsar {

namespace dbus {

class server : public std::enable_shared_from_this<server> {
    DBus::Dispatcher::pointer dispatcher = DBus::Dispatcher::create();
    DBus::Connection::pointer connection;

    public:
    const std::string bus_name;
    server(const std::string bus_name_in);
    ~server();
    void start();
};

void init();
std::shared_ptr<server> get_server();

} // namespace dbus

} // namespace pulsar

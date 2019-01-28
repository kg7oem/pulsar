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

#include <dbus-c++/dbus.h>

#include <pulsar/dbus.adaptor.h>
#include <pulsar/thread.h>
#include <pulsar/types.h>

#define PULSAR_DBUS_NAME "audio.pulsar"
#define PULSAR_DBUS_DOMAIN_PREFIX "/Domain/"
#define PULSAR_DBUS_NODE_PREFIX "/Node/"

namespace pulsar {

namespace dbus {

void init();
DBus::Connection & get_connection();

struct server {
    const string_type bus_name;
    std::thread * dispatcher_thread = nullptr;
    DBus::Connection connection = DBus::Connection::SessionBus();

    server(const string_type& bus_name_in);
    void start();
};

} // namespace dbus

} // namespace pulsar

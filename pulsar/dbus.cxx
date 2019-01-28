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

#include <cassert>

#include <pulsar/dbus.h>
#include <pulsar/system.h>

namespace pulsar {

namespace dbus {

DBus::BusDispatcher * global_dispatcher = nullptr;
server * global_server = nullptr;

void init()
{
    assert(global_server == nullptr);
    assert(global_server == nullptr);

    global_dispatcher = new DBus::BusDispatcher();
    DBus::default_dispatcher = global_dispatcher;

    global_server = new server(PULSAR_DBUS_NAME);
    global_server->start();
}

DBus::Connection& get_connection()
{
    assert(global_server != nullptr);
    return (global_server->connection);
}

server::server(const string_type& bus_name_in)
: bus_name(bus_name_in)
{
    connection.request_name(bus_name.c_str());
}

void server::start()
{
    if (dispatcher_thread != nullptr) {
        system_fault("attempt to start DBus server twice");
    }

    dispatcher_thread = new thread_type([]() -> void { global_dispatcher->enter(); });
}

} // namespace dbus

} // namespace pulsar

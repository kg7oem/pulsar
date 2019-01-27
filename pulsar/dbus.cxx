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

namespace pulsar {

namespace dbus {

std::shared_ptr<server> global_server;

void init()
{
    assert(global_server == nullptr);
    global_server = std::make_shared<server>(PULSAR_DBUS_NAME);
}

server::server(const std::string bus_name_in)
: bus_name(bus_name_in)
{ }

server::~server()
{
    // if (dispatcher_thread != nullptr) {
    //     dbus_dispatcher->leave();
    //     std::cout << "joining with DBUS dispatcher thread" << std::endl;
    //     dispatcher_thread->join();
    //     std::cout << "done joining with dispatcher thread" << std::endl;
    //     delete dispatcher_thread;
    //     dispatcher_thread = nullptr;
    // }
}

void server::start()
{
    connection->request_name(PULSAR_DBUS_NAME);
}

std::shared_ptr<server> get_server()
{
    assert(global_server != nullptr);
    return global_server;
}

} // namespace dbus

} // namespace pulsar

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

#include <memory>
#include <iostream>

#include "domain.h"
#include "node.h"

using namespace std;

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024

int main(void)
{
    auto domain = make_shared<pulsar::domain>("main", SAMPLE_RATE, BUFFER_SIZE);

    auto node1 = domain->make_node<pulsar::node>("one");
    node1->add_output("Output");

    auto node2 = domain->make_node<pulsar::node>("two");
    node2->add_input("Input");

    node1->get_output("Output")->connect(node2->get_input("Input"));

    domain->step();
}

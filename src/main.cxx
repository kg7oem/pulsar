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
#include "jackaudio.h"
#include "node.h"

using namespace std;
using namespace std::chrono_literals;

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define NUM_THREADS 4

int main(void)
{
    auto domain = make_shared<pulsar::domain>("main", SAMPLE_RATE, BUFFER_SIZE);

    auto jack = domain->make_node<pulsar::jackaudio::node>("jack1");
    jack->audio.add_output("Output");
    jack->audio.add_input("Input");

    // auto jack2 = domain->make_node<pulsar::jackaudio::node>("jack2");
    // jack2->open("jack2");
    // jack2->audio.add_output("Output");
    // jack2->audio.add_input("Input");

    auto node1 = domain->make_node<pulsar::dummy_node>("why not");
    node1->audio.add_input("Input");
    node1->audio.add_output("Output");

    auto node2 = domain->make_node<pulsar::dummy_node>("intermediate 1");
    node2->audio.add_input("Input");
    node2->audio.add_output("Output");

    auto node3 = domain->make_node<pulsar::dummy_node>("intermediate 2");
    node3->audio.add_input("Input");
    node3->audio.add_output("Output");

    auto node4 = domain->make_node<pulsar::dummy_node>("intermediate 3");
    node4->audio.add_input("Input");
    node4->audio.add_output("Output");

    auto node5 = domain->make_node<pulsar::dummy_node>("multiple inputs");
    node5->audio.add_input("Input 1");
    node5->audio.add_input("Input 2");
    node5->audio.add_input("Input 3");
    node5->audio.add_output("Output");

    auto node6 = domain->make_node<pulsar::dummy_node>("mixed outputs");
    node6->audio.add_input("Input");
    node6->audio.add_output("Output");

    jack->audio.get_output("Output")->connect(node1->audio.get_input("Input"));
    // jack2->audio.get_output("Output")->connect(node1->audio.get_input("Input"));

    node1->audio.get_output("Output")->connect(node2->audio.get_input("Input"));
    node1->audio.get_output("Output")->connect(node3->audio.get_input("Input"));
    node1->audio.get_output("Output")->connect(node4->audio.get_input("Input"));

    node2->audio.get_output("Output")->connect(node5->audio.get_input("Input 1"));
    node3->audio.get_output("Output")->connect(node5->audio.get_input("Input 2"));
    node4->audio.get_output("Output")->connect(node5->audio.get_input("Input 3"));

    node2->audio.get_output("Output")->connect(node6->audio.get_input("Input"));
    node5->audio.get_output("Output")->connect(node6->audio.get_input("Input"));

    jack->audio.get_input("Input")->connect(node6->audio.get_output("Output"));
    // jack2->audio.get_input("Input")->connect(node6->audio.get_output("Output"));

    domain->activate(NUM_THREADS);

    while(1) {
        std::this_thread::sleep_for(1s);
    }
}

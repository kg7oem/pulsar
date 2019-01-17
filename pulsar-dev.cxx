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
#include <unistd.h>

#include "pulsar/async.h"
#include "pulsar/domain.h"
#include "pulsar/jackaudio.h"
#include "pulsar/ladspa.h"
#include "pulsar/logging.h"
#include "pulsar/property.h"
#include "pulsar/system.h"

using namespace std;
using namespace std::chrono_literals;

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define NUM_THREADS 4
#define ALARM_TIMEOUT 1

static void init_logging()
{
    auto logging = logjam::logengine::get_engine();
    auto console = make_shared<logjam::logconsole>(logjam::loglevel::debug);

    logging->add_destination(console);
    logging->start();
}

static void init_pulsar()
{
    pulsar::async::init();

    pulsar::async::register_tick_handler([] (void *) {
        alarm(ALARM_TIMEOUT);
    });

    alarm(ALARM_TIMEOUT);
}

UNUSED static void init()
{
    init_logging();
}

UNUSED static void process_audio()
{
    init_pulsar();

    auto domain = pulsar::domain::make("main", SAMPLE_RATE, BUFFER_SIZE);
    auto gain_left = domain->make_node<pulsar::ladspa::node>("left", "/usr/lib/ladspa/amp.so", 1048);
    auto gain_right = domain->make_node<pulsar::ladspa::node>("right", "/usr/lib/ladspa/amp.so", 1048);
    auto jack = domain->make_node<pulsar::jackaudio::node>("pulsar");

    jack->audio.add_output("in_left")->connect(gain_left->audio.get_input("Input"));
    jack->audio.add_output("in_right")->connect(gain_right->audio.get_input("Input"));

    jack->audio.add_input("out_left")->connect(gain_left->audio.get_output("Output"));
    jack->audio.add_input("out_right")->connect(gain_right->audio.get_output("Output"));

    domain->activate(NUM_THREADS);

    while(1) {
        std::this_thread::sleep_for(1s);
    }
}

int main(void)
{
    init();

    log_info("pulsar-dev initialized");
    log_info("Using Boost ", pulsar::system::get_boost_version());

    pulsar::property::integer param_integer("integer");
    param_integer.set(10);

    log_debug("Value: ", param_integer.get());
    log_debug("As string: ", param_integer.to_str());

    return 0;
}

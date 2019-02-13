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

#include <pulsar/logging.h>
#include <pulsar/pulseaudio.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace pulseaudio {

using namespace std::chrono_literals;

static pa_mainloop_api * pulse_api = nullptr;
static thread_type * pulse_thread = nullptr;

static void pulse_loop()
{
    assert(pulse_api == nullptr);

    auto loop = pa_mainloop_new();
    assert(loop != nullptr);

    pulse_api = pa_mainloop_get_api(loop);
    assert(pulse_api != nullptr);

    log_debug("pulseaudio thread is giving control to pulseaudio main loop");
    auto result = pa_mainloop_run(loop, NULL);
    log_debug("pulseaudio thread got control back from pulseaudio main loop");

    if (result < 0) {
        system_fault("pulseaudio pa_mainloop_run() had an error response");
    }
}

void init()
{
    log_debug("pulseaudio system is initializing");

    assert(pulse_thread == nullptr);

    pulse_thread = new thread_type(pulse_loop);
}

pa_context * make_context(const string_type& name_in)
{
    assert(pulse_api != nullptr);

    auto context = pa_context_new(pulse_api, name_in.c_str());
    assert(context != nullptr);

    return context;
}

} // namespace pulseaudio

} // namespace pulsar

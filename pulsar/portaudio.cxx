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

#include <pulsar/audio.util.h>
#include <pulsar/async.h>
#include <pulsar/debug.h>
#include <pulsar/logging.h>
#include <pulsar/portaudio.h>

namespace pulsar {

namespace portaudio {

static mutex_type portaudio_mutex;

void init()
{
    log_debug("Initializing portaudio support");

    {
        auto lock = debug_get_lock(portaudio_mutex);
        auto err = Pa_Initialize();

        if (err != paNoError) {
            system_fault("Could not initialize portaudio: ",  Pa_GetErrorText(err));
        }
    }

    library::register_node_factory("pulsar::portaudio::node", make_node);
}

pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<portaudio::node>(name_in);
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::io(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).value->set("pulsar::portaudio::node");
}

node::~node()
{ }

void node::execute()
{ }

void node::init()
{
    log_trace("portaudio init() was invoked");

    add_input("left_out");
    add_input("right_out");

    add_output("right_in");
    add_output("left_in");

    pulsar::node::io::init();
}

void node::add_input(const string_type& name_in)
{
    auto buffer = audio::buffer::make();
    buffer->init(domain->buffer_size);
    buffers.push_back(buffer);

    sends[name_in] = buffer->get_pointer();
    audio.add_input(name_in);
}

void node::add_output(const string_type& name_in)
{
    auto buffer = audio::buffer::make();
    buffer->init(domain->buffer_size);
    buffers.push_back(buffer);

    receives[name_in] = buffer->get_pointer();
    audio.add_output(name_in);
}

static int process_cb(const void *inputBuffer, void *outputBuffer, size_type framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userdata )
{
    log_trace("***************** portaudio process callback was invoked");
    assert(userdata != nullptr);

    promise_type<void> promise;
    // FIXME is this the right way to do this? It is much cleaner than the jackaudio node
    // way of handling callbacks
    auto node = (portaudio::node *) userdata;

    log_trace("creating async job to invoke portaudio::node::process_cb()");
    async::submit_job([&] {
        log_trace("async job started");
        node->process_cb(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);

        log_trace("async job completed; setting promise value");
        promise.set_value();
    });

    log_trace("waiting for async job to complete");
    promise.get_future().get();

    log_trace("***************** giving control back to portaudio");
    return 0;
}

void node::process_cb(const void *inputBuffer, void *outputBuffer, size_type framesPerBuffer, const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags statusFlags)
{
    log_trace("portaudio::node::process_cb() was invoked");
    assert(framesPerBuffer == domain->buffer_size);

    if (statusFlags) {
        system_fault("portaudio callback got non-zero statusFlags: ", statusFlags);
    }

    std::vector<sample_type *> input;
    input.push_back(receives["left_in"]);
    input.push_back(receives["right_in"]);
    audio::util::pcm_deinterlace(input, static_cast<const sample_type *>(inputBuffer), framesPerBuffer);

    process(receives, sends);

    std::vector<sample_type *> output;
    output.push_back(sends["left_out"]);
    output.push_back(sends["right_out"]);
    audio::util::pcm_interlace(static_cast<sample_type *>(outputBuffer), output, framesPerBuffer);
}

void node::activate()
{
    log_trace("portaudio activate() was invoked");
    assert(stream == nullptr);

    {
        auto lock = debug_get_lock(portaudio_mutex);
        auto userdata = static_cast<void *>(this);
        auto err = Pa_OpenDefaultStream(&stream, 2, 2, paFloat32, domain->sample_rate, domain->buffer_size, portaudio::process_cb, userdata);

        if (err != paNoError) {
            system_fault("Could not open portaudio default stream: ", Pa_GetErrorText(err));
        }
    }

    pulsar::node::io::activate();
}

void node::start()
{
    log_trace("portaudio start() was invoked");
    assert(stream != nullptr);

    {
        auto lock = debug_get_lock(portaudio_mutex);
        auto err = Pa_StartStream(stream);

        if (err != paNoError) {
            system_fault("Could not start portaudio stream: ", Pa_GetErrorText(err));
        }
    }

    pulsar::node::io::start();
}

void node::stop()
{
    log_trace("portaudio stop() was invoked");
    assert(stream != nullptr);

    {
        auto lock = debug_get_lock(portaudio_mutex);
        auto err = Pa_StopStream(stream);

        if (err != paNoError) {
            system_fault("Could not stop portaudio stream: ", Pa_GetErrorText(err));
        }

    }

    pulsar::node::io::stop();
}

} // namespace portaudio

} // namespace pulsar

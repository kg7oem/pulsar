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
#include <pulsar/logging.h>
#include <pulsar/portaudio.h>

namespace pulsar {

namespace portaudio {

static mutex_type portaudio_mutex;

void init()
{
    log_debug("Initializing portaudio support");

    {
        log_trace("inside portaudio init lock block");
        auto lock = pulsar_get_lock(portaudio_mutex);

        log_trace("calling Pa_Initialize()");
        auto err = Pa_Initialize();
        log_trace("done calling Pa_Initialize()");

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

void node::add_send(const string_type& name_in)
{
    auto buffer = audio::buffer::make();
    buffer->init(domain->buffer_size);
    buffers.push_back(buffer);

    sends[name_in] = buffer->get_pointer();
}

void node::add_receive(const string_type& name_in)
{
    auto buffer = audio::buffer::make();
    buffer->init(domain->buffer_size);
    buffers.push_back(buffer);

    receives[name_in] = buffer->get_pointer();
}

static int process_cb(const void *inputBuffer, void *outputBuffer, size_type framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userdata )
{
    log_trace("***************** portaudio process callback was invoked");
    assert(userdata != nullptr);

    // FIXME is this the right way to do this? It is much cleaner than the jackaudio node
    // way of handling callbacks
    auto node = (portaudio::node *) userdata;
    node->process_cb(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);

    log_trace("***************** giving control back to portaudio");
    return 0;
}

void node::process_cb(const void *inputBuffer, void *outputBuffer, size_type framesPerBuffer, const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags statusFlags)
{
    log_trace("portaudio::node::process_cb() was invoked");
    assert(framesPerBuffer == domain->buffer_size);

    if (statusFlags) {
        if (statusFlags & paPrimingOutput) {
            log_trace("portaudio stream is priming for node ", name);
            // discard anything that is going to be used for priming
            return;
        }

        if (statusFlags & paInputUnderflow) {
            statusFlags &= ~paInputUnderflow;
            log_error("portaudio input underflow for node ", name);
        }

        if (statusFlags & paInputOverflow) {
            statusFlags &= ~paInputOverflow;
            log_error("portaudio input overflow for node ", name);
        }

        if (statusFlags & paOutputUnderflow) {
            statusFlags &= ~paOutputUnderflow;
            log_error("portaudio output underflow for node ", name);
        }

        if (statusFlags & paOutputOverflow) {
            statusFlags &= ~paOutputOverflow;
            log_error("portaudio output overflow for node ", name);
        }

        if (statusFlags) {
            system_fault("portaudio callback got unknown statusFlags: ", statusFlags);
        }
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

    auto num_sends = audio.get_input_names().size();
    auto num_receives = audio.get_output_names().size();

    if (num_sends == 0 || num_receives == 0) {
        system_fault("portaudio nodes must have at least 1 send and 1 receive");
    }

    for(auto&& name : audio.get_output_names()) {
        log_debug("adding receive to portaudio node: ", name);
        add_receive(name);
    }

    for(auto&& name : audio.get_input_names()) {
        log_debug("adding send to portaudio node: ", name);
        add_send(name);
    }

    {
        auto lock = pulsar_get_lock(portaudio_mutex);
        auto userdata = static_cast<void *>(this);
        auto err = Pa_OpenDefaultStream(&stream, num_receives, num_sends, paFloat32, domain->sample_rate, domain->buffer_size, portaudio::process_cb, userdata);

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
        auto lock = pulsar_get_lock(portaudio_mutex);
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
        auto lock = pulsar_get_lock(portaudio_mutex);
        auto err = Pa_StopStream(stream);

        if (err != paNoError) {
            system_fault("Could not stop portaudio stream: ", Pa_GetErrorText(err));
        }

    }

    pulsar::node::io::stop();
}

} // namespace portaudio

} // namespace pulsar

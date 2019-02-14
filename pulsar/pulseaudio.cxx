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

#include <boost/detail/endian.hpp>

#include <pulsar/library.h>
#include <pulsar/logging.h>
#include <pulsar/pulseaudio.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>

namespace pulsar {

namespace pulseaudio {

static pa_threaded_mainloop  * pulse_main_loop = nullptr;
static pa_mainloop_api * pulse_api = nullptr;

static void lock_pulse()
{
    assert(pulse_main_loop != nullptr);
    log_trace("getting pulse lock");
    pa_threaded_mainloop_lock(pulse_main_loop);
    log_trace("got pulse lock");
}

static void unlock_pulse()
{
    assert(pulse_main_loop != nullptr);
    log_trace("giving up pulse lock");
    pa_threaded_mainloop_unlock(pulse_main_loop);
    log_trace("gave up pulse lock");
}

pulsar::node::base * make_client_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<pulseaudio::client_node>(name_in);
}

void init()
{
    log_debug("pulseaudio system is initializing");

    assert(pulse_api == nullptr);
    assert(pulse_main_loop == nullptr);

    pulse_main_loop = pa_threaded_mainloop_new();
    assert(pulse_main_loop != nullptr);

    pulse_api = pa_threaded_mainloop_get_api(pulse_main_loop);
    assert(pulse_api != nullptr);

    // will have to lock pulseaudio objects after the loop has been started
    auto result = pa_threaded_mainloop_start(pulse_main_loop);

    if (result < 0) {
        system_fault("could not start pulseaudio threaded mainloop");
    }

    library::register_node_factory("pulsar::pulseaudio::client", make_client_node);
}

pa_context * make_context(const string_type& name_in)
{
    assert(pulse_api != nullptr);

    lock_pulse();

    auto context = pa_context_new(pulse_api, name_in.c_str());
    assert(context != nullptr);

    unlock_pulse();

    return context;
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).set("pulsar::jackaudio::node");

    add_property("config:context", property::value_type::string).set(name_in);
    add_property("config:sample_rate", property::value_type::size);
}

pa_stream * node::make_stream(const string_type& name_in, const pa_sample_spec * spec_in)
{
    lock_pulse();

    assert(context != nullptr);

    auto stream = pa_stream_new(context, name_in.c_str(), spec_in, nullptr);

    if (stream == nullptr) {
        auto error = pa_context_errno(context);
        system_fault("could not create new pulseaudio stream: ", pa_strerror(error));
    }

    unlock_pulse();

    return stream;
}

static void state_cb(pa_context *context, void *userdata)
{
    auto notifier = static_cast<node::state_notifier_type *>(userdata);
    auto state = pa_context_get_state(context);
    notifier->notify(state);
}

static void stream_write_cb(pa_stream * stream_in, size_t bytes_in , void * userdata_in)
{
    log_trace("stream write callback invoked; bytes_in = ", bytes_in);

    auto node = static_cast<pulseaudio::node *>(userdata_in);

    auto zeros = node->get_domain()->get_zero_buffer();
    pa_stream_write(stream_in, zeros->get_pointer(), bytes_in, NULL, 0, PA_SEEK_RELATIVE);
}

static void stream_read_cb(pa_stream * /* stream */, size_t bytes_in, void * /* userdata */)
{
    system_fault("stream read callback invoked; bytes_in = ", bytes_in);
}

void node::activate()
{
    log_trace("starting pulseaudio activation for node: ", name);
    assert(context == nullptr);

#if defined BOOST_BIG_ENDIAN
    output_spec.format = input_spec.format = PA_SAMPLE_FLOAT32BE;
#elif defined BOOST_LITTLE_ENDIAN
    output_spec.format = input_spec.format = PA_SAMPLE_FLOAT32LE;
#else
#error system is not big or little endian?
#endif

    output_spec.rate = input_spec.rate = get_property("config:sample_rate").get_size();

    output_spec.channels = 2;
    input_spec.channels = 2;

    buffer_attr.fragsize = (uint32_t)-1;
    buffer_attr.maxlength = domain->buffer_size * sizeof(sample_type);
    buffer_attr.minreq = 0;
    buffer_attr.prebuf = (uint32_t)-1;
    buffer_attr.tlength = domain->buffer_size * sizeof(sample_type);

    context = make_context(get_property("config:context").get_string());

    lock_pulse();

    pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_context_set_state_callback(context, state_cb, &context_state);

    unlock_pulse();

    log_debug("waiting for pulseaudio context to become ready");
    context_state.wait_for(PA_CONTEXT_READY);
    log_debug("pulseaudio context is now ready");

    input_stream = make_stream("Record", &input_spec);
    output_stream = make_stream("Playback", &output_spec);

    lock_pulse();
    pa_stream_set_read_callback(input_stream, stream_read_cb, nullptr);
    pa_stream_set_write_callback(output_stream, stream_write_cb, this);
    unlock_pulse();
}

void node::start()
{
    log_trace("pulseaudio node is starting: ", name);

    lock_pulse();

    auto flags = static_cast<pa_stream_flags_t>(
        PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE
    );

    auto result = pa_stream_connect_record(input_stream, nullptr, &buffer_attr, flags);

    if (result < 0) {
        system_fault("could not connect record stream");
    }

    result = pa_stream_connect_playback(output_stream, nullptr, &buffer_attr, flags, nullptr, nullptr);

    if (result < 0) {
        system_fault("could not connect playback stream");
    }

    unlock_pulse();

    pulsar::node::base::start();
}

client_node::client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: node(name_in, domain_in)
{ }

} // namespace pulseaudio

} // namespace pulsar

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

static pa_mainloop * pulse_main_loop = nullptr;
static pa_mainloop_api * pulse_api = nullptr;
static thread_type * pulse_thread = nullptr;

static void pulse_loop()
{
    assert(pulse_main_loop != nullptr);

    log_debug("pulseaudio thread is giving control to pulseaudio main loop");
    auto result = pa_mainloop_run(pulse_main_loop, NULL);
    log_debug("pulseaudio thread got control back from pulseaudio main loop");

    if (result < 0) {
        system_fault("pulseaudio pa_mainloop_run() had an error response");
    }
}

pulsar::node::base * make_client_node(const string_type& name_in, std::shared_ptr<domain> domain_in)
{
    return domain_in->make_node<pulseaudio::client_node>(name_in);
}

void init()
{
    log_debug("pulseaudio system is initializing");

    assert(pulse_thread == nullptr);
    assert(pulse_api == nullptr);
    assert(pulse_main_loop == nullptr);

    pulse_main_loop = pa_mainloop_new();
    assert(pulse_main_loop != nullptr);

    pulse_api = pa_mainloop_get_api(pulse_main_loop);
    assert(pulse_api != nullptr);

    pulse_thread = new thread_type(pulse_loop);

    library::register_node_factory("pulsar::pulseaudio::client", make_client_node);
}

pa_context * make_context(const string_type& name_in)
{
    assert(pulse_api != nullptr);

    auto context = pa_context_new(pulse_api, name_in.c_str());
    assert(context != nullptr);

    return context;
}

node::node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: pulsar::node::base(name_in, domain_in)
{
    add_property("node:class", property::value_type::string).set("pulsar::jackaudio::node");

    add_property("config:context", property::value_type::string).set(name_in);
    add_property("config:sample_rate", property::value_type::size);
}

static void state_cb(pa_context *context, void *userdata)
{
    auto notifier = static_cast<node::state_notifier_type *>(userdata);
    auto state = pa_context_get_state(context);
    notifier->notify(state);
}

static void stream_request_cb(pa_stream * /* stream */, size_t /* length */ , void * /* userdata */)
{
    system_fault("stream request cb invoked");
}

void node::activate()
{
    log_trace("starting pulseaudio activation for node: ", name);

    assert(context == nullptr);
    context = make_context(get_property("config:context").get_string());

    pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_context_set_state_callback(context, state_cb, &context_state);

    log_debug("waiting for pulseaudio context to become ready");
    context_state.wait_for(PA_CONTEXT_READY);
    log_debug("pulseaudio context is now ready");

#if defined BOOST_BIG_ENDIAN
    sample_spec.format = PA_SAMPLE_FLOAT32BE;
#elif defined BOOST_LITTLE_ENDIAN
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
#else
#error system is not big or little endian?
#endif //

    sample_spec.rate = get_property("config:sample_rate").get_size();
    sample_spec.channels = 1;

    stream = pa_stream_new(context, "Playback", &sample_spec, nullptr);
    assert(stream != nullptr);

    pa_stream_set_write_callback(stream, stream_request_cb, nullptr);

    buf_attr.fragsize = (uint32_t)-1;
    buf_attr.maxlength = domain->buffer_size;
    buf_attr.minreq = 0;
    buf_attr.prebuf = (uint32_t)-1;
    buf_attr.tlength = domain->buffer_size;

    auto result = pa_stream_connect_playback(
        stream, nullptr, &buf_attr,
        static_cast<pa_stream_flags_t>(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE),
        nullptr, nullptr
    );

    if (result < 0) {
        system_fault("could not connect playback stream");
    }

    pulsar::node::base::activate();
}

void node::context_ready()
{
    system_fault("context is ready");
}

client_node::client_node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in)
: node(name_in, domain_in)
{ }

} // namespace pulseaudio

} // namespace pulsar

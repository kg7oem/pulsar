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

#pragma once

#include <pulsar/audio.h>
#include <pulsar/library.h>
#include <pulsar/node.h>

namespace pulsar {

namespace portaudio {

#include <portaudio.h>

void init();
pulsar::node::base * make_node(const string_type& name_in, std::shared_ptr<domain> domain_in);

class node : public pulsar::node::io {
    protected:
    PaStream *stream = nullptr;
    std::map<string_type, sample_type *> receives, sends;
    std::vector<std::shared_ptr<audio::buffer>> buffers;

    void add_send(const string_type& name_in);
    void add_receive(const string_type& name_in);
    void start() override;
    void execute() override;
    virtual void stop() override;

    public:
    node(const string_type& name_in, std::shared_ptr<pulsar::domain> domain_in);
    ~node();
    virtual void activate() override;
    void process_cb(const void *inputBuffer, void *outputBuffer, size_type framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
};

} // namespace portaudio

} // namespace pulsar

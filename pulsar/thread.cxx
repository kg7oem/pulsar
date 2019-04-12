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

#include <pthread.h>
#include <string.h>

#include <pulsar/logging.h>
#include <pulsar/system.h>
#include <pulsar/thread.h>
#include <pulsar/types.h>

namespace pulsar {

namespace thread {

// from https://stackoverflow.com/a/31652324
void set_realtime_priority(thread_type& thread_in, const rt_priorty& priority_in)
{
    sched_param sch_params;
    sch_params.sched_priority = static_cast<int>(priority_in);

    if (auto error = pthread_setschedparam(thread_in.native_handle(), SCHED_RR, &sch_params)) {
        log_error("could not set thread to realtime priority: ", strerror(error));
    }
}

} // namespace thread

} // namespace pulsar

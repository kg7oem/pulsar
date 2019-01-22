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

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <chrono>
#include <ctime>
#include <list>
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <vector>

#ifdef LOGJAM_LOG_MACROS
#ifndef LOGJAM_NLOG
#define LOGJAM_LOG_VARGS(logname, loglevel, ...) logjam::send_vargs_logevent(logname, loglevel, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define LOGJAM_LOG_LAMBDA(logname, loglevel, block) logjam::send_lambda_logevent(logname, loglevel, __PRETTY_FUNCTION__, __FILE__, __LINE__, [&]() -> std::string block)
#else // LOGJAM_NLOG
#define LOGJAM_LOG_VARGS(logname, loglevel, ...) ((void)0)
#define LOGJAM_LOG_LAMBDA(logname, loglevel, block) ((void)0)
#endif // LOGJAM_NLOG
#endif // LOGJAM_LOG_MACROS

namespace logjam {

using namespace std::chrono_literals;

using log_wrapper_type = std::function<std::string ()>;

enum class loglevel {
    uninit = -2,
    none = -1,
    unknown = 0,
    trace = 10,
    debug = 30,
    verbose = 35,
    info = 40,
    error = 80,
    fatal = 100,
};

struct logevent;
class logengine;

class mutex : public std::mutex {
    private:
        std::thread::id owned_by;

    public:
        void lock();
        void unlock();
        bool caller_has_lock();
};

class shared_mutex : public std::shared_timed_mutex {
    private:
        std::thread::id owned_exclusive_by;
        std::mutex lock_tracking_mutex;
        std::unordered_set<std::thread::id> shared_owners;

    public:
        void lock();
        void unlock();
        // true if the caller has a write lock
        bool caller_has_lockex();
        void lock_shared();
        void unlock_shared();
        // true if caller has at least a read lock - it can also
        // have a write lock and be true
        bool caller_has_locksh();
};

class lockable {
    using lock = std::unique_lock<logjam::mutex>;

    private:
        logjam::mutex lock_mutex;

    protected:
        lock get_lock();
        bool caller_has_lock();
};

class shareable {
    using mutex = shared_mutex;
    using write_lock = std::unique_lock<mutex>;
    using read_lock = std::shared_lock<mutex>;

    private:
        mutex lock_mutex;

    protected:
        write_lock get_lockex();
        read_lock get_locksh();
        bool caller_has_lockex();
        bool caller_has_locksh();
};

struct logsource {
    const char* c_str;
    logsource(const char* c_str_in);
    bool operator==(const char* rhs) const;
    bool operator==(const logsource& rhs) const;
};

// all the members of a logevent are const for thread safety
struct logevent {
    using timestamp = std::chrono::time_point<std::chrono::system_clock>;

    const std::string source;
    const loglevel level = loglevel::uninit;
    const timestamp when;
    const std::thread::id tid;
    const char *function = nullptr;
    const char *file = nullptr;
    const int32_t line = -1;
    const std::string message;

    logevent(const std::string& source_in, const loglevel& level_in, const timestamp& when, const std::thread::id& tid_in, const char* function, const char *file, const int& line, const std::string& message_in);
    ~logevent() = default;
};

struct baseobj {
    baseobj(const baseobj&) = delete;
    baseobj(const baseobj&&) = delete;
    baseobj& operator=(const baseobj&);

    baseobj() = default;
    ~baseobj() = default;
};

class logdest : public baseobj {
    friend class logengine;
    using destid = uint32_t;

    private:
        std::atomic<loglevel> min_level;
        loglevel set_min_level__lockreq(const loglevel& min_level_in);
        static destid next_destination_id();

    protected:
        logengine* engine = nullptr;
        virtual void handle_output(const logevent& event) = 0;
        loglevel get_min_level();

    public:
        const destid id = logdest::next_destination_id();
        logdest(const loglevel& min_level_in);
        virtual ~logdest() = default;
        loglevel set_min_level(const loglevel& min_level_in);
        void output(const logevent& event_in);
};

class logengine : public baseobj, shareable {
    using lockfree_queue = boost::lockfree::queue<logevent*>;

    friend loglevel logdest::set_min_level(const loglevel& min_level_in);
    friend loglevel logdest::set_min_level__lockreq(const loglevel& min_level_in);

    private:
        std::vector<std::shared_ptr<logdest>> destinations;
        lockfree_queue event_buffer{0};
        loglevel get_min_level();
        loglevel set_min_level__lockex(loglevel level_in);
        void update_min_level__lockex(void);
        void add_destination__lockex(const std::shared_ptr<logdest>& destination_in);
        void deliver__locksh(const logevent& event_in);
        void deliver_to_one__locksh(const std::shared_ptr<logdest>& dest_in, const logevent& event_in);
        void deliver_to_all__locksh(const logevent& event_in);
        void start__lockex();

    protected:
        std::atomic<loglevel> min_log_level = ATOMIC_VAR_INIT(loglevel::none);
        bool buffer_events = true;
        // messages will only be delivered when started
        bool started = false;

    public:
        logengine() = default;
        // get the singleton instance
        static logengine* get_engine();
        void update_min_level(void);
        void add_destination(const std::shared_ptr<logdest>& destination_in);
        bool should_log(const loglevel& level_in);
        void deliver(const logevent& event);
        void start();
};

class logconsole : public logdest, lockable {
    private:
        virtual void handle_output(const logevent& event_in) override;
        void write_stdio__lockreq(const std::string& message_in);

    public:
        logconsole(const loglevel& level_in = loglevel::debug)
            : logdest(level_in) { }
        virtual ~logconsole() = default;
        virtual std::string format_event(const logevent& event) const;
};

class logmemory : public logdest, lockable {
    private:
        // ordered with oldest event at the start
        std::list<logevent> event_history;
        std::mutex event_history_mutex;
        std::chrono::milliseconds max_age = 0ms;
        virtual void handle_output(const logevent& event_in) override;
        void cleanup();

    public:
        logmemory(const loglevel& level_in)
            : logdest(level_in) { }
        virtual ~logmemory() = default;
        std::chrono::milliseconds get_max_age();
        void set_max_age(std::chrono::milliseconds max_age_in);
        std::list<logevent> get_event_history();
        std::vector<std::string> format_event_history();
};

const char* level_name(const loglevel& level_in);
loglevel level_from_name(const char* name_in);
loglevel level_from_name(const std::string& name_in);
bool should_log(const loglevel& level_in);

template <typename T>
void sstream_accumulate_vaargs(std::stringstream& sstream, T&& t) {
    sstream << t;
}

template <typename T, typename... Args>
void sstream_accumulate_vaargs(std::stringstream& sstream, T&& t, Args&&... args) {
    sstream_accumulate_vaargs(sstream, t);
    sstream_accumulate_vaargs(sstream, args...);
}

template <typename... Args>
std::string vaargs_to_string(Args&&... args) {
    std::stringstream buf;
    sstream_accumulate_vaargs(buf, args...);
    return buf.str();
}

template<typename... Args>
void send_vargs_logevent(const std::string& source, const loglevel& level, const char *function, const char *path, const int& line, Args&&... args)
{
    if (logjam::should_log(level)) {
        auto when = std::chrono::system_clock::now();

        auto tid = std::this_thread::get_id();
        auto message = vaargs_to_string(args...);
        logevent event(source, level, when, tid, function, path, line, message);
        logengine::get_engine()->deliver(event);
    }

    return;
}

void send_lambda_logevent(const std::string& source, const loglevel& level, const char *function, const char *path, const int& line, const log_wrapper_type& lambda_in);

std::string format_event_detailed(const logevent& event_in);

} // namespace logjam

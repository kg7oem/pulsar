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

#include <cassert>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>

#include <logjam/logjam.h>

#ifdef LOGJAM_NDEBUG
#ifndef NDEBUG
#define NDEBUG
#endif // NDEBUG
#endif

#ifdef NDEBUG
#define LOGJAM_NLOCK_ASSERTIONS
#endif // NDEBUG

#ifdef LOGJAM_NLOCK_ASSERTIONS
#define assert_lock(expr) ((void)0)
#else // LOGJAM_LOCK_ASSERTIONS
#define assert_lock(expr) assert(expr)
#endif // LOGJAM_LOCK_ASSERTIONS

// g++ 6.3.0 as it comes in debian/stretch does not support maybe_unused
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED [[ maybe_unused ]]
#endif

namespace logjam {

std::string format_event_detailed(const logevent& event_in) {
    std::stringstream buf;

    #if 0
    buf << event_in.tid << " ";
    buf << event_in.source << "." << level_name(event_in.level) << " ";
    buf << event_in.function << std::endl;
    #endif

    #if 1
    // might be a full path name
    auto file_name = std::string(event_in.file);
    auto pos = file_name.find_last_of("/");

    if (pos != std::string::npos) {
        file_name = file_name.substr(pos + 1);
    }

    buf << event_in.tid << " ";
    buf << file_name << ":" << event_in.line << " ";
    buf << event_in.message;
    #endif

    auto strbuf = buf.str();
    auto last_char_pos = strbuf.size() - 1;
    auto last_char = strbuf[last_char_pos];

    // FIXME this should look for newlines inside the message and
    // if the newline is not at the end of the message then put
    // two spaces after every newline so the message is indented when
    // the user views it - this makes it easier to read and prevents
    // faking output from the log system to the end user with carefully
    // crafted message lengths and line wrapping

    // only add a new line if the message does not already have one
    // FIXME this won't work right on Windows
    // FIXME what about UTF-16 and UTF-32?
    if (last_char != '\n') {
        strbuf += "\n";
    }

    return strbuf;
}

bool should_log(const loglevel& level_in) {
    return logengine::get_engine()->should_log(level_in);
}

// THREAD this function is thread safe
const char* level_name(const loglevel& level_in) {
    switch (level_in) {
        case loglevel::uninit:  return "(uninitialized)";
        case loglevel::none:    return "(none)";
        case loglevel::unknown: return "unknown";
        case loglevel::trace:   return "trace";
        case loglevel::debug:   return "debug";
        case loglevel::verbose: return "verbose";
        case loglevel::info:    return "info";
        case loglevel::error:   return "error";
        case loglevel::fatal:   return "fatal";
    }

    throw std::runtime_error("switch() failure");
}

loglevel level_from_name(const std::string& name_in) {
    return level_from_name(name_in.c_str());
}

loglevel level_from_name(const char* name_in) {
    if (strcmp(name_in, "(uninitialized)") == 0) {
        return loglevel::uninit;
    } else if (strcmp(name_in, "(none)") == 0) {
        return loglevel::none;
    } else if (strcmp(name_in, "unknown") == 0) {
        return loglevel::unknown;
    } else if (strcmp(name_in, "trace") == 0) {
        return loglevel::trace;
    } else if (strcmp(name_in, "debug") == 0) {
        return loglevel::debug;
    } else if (strcmp(name_in, "verbose") == 0) {
        return loglevel::verbose;
    } else if (strcmp(name_in, "info") == 0) {
        return loglevel::info;
    } else if (strcmp(name_in, "error") == 0) {
        return loglevel::error;
    } else if (strcmp(name_in, "fatal") == 0) {
        return loglevel::fatal;
    }

    std::string buf("no match for loglevel name: ");
    buf += name_in;
    throw std::runtime_error(buf);
}

void send_lambda_logevent(const std::string& source, const loglevel& level, const char *function, const char *path, const int& line, const log_wrapper_type& lambda_in) {
    if (logjam::should_log(level)) {
        auto when = std::chrono::system_clock::now();
        auto tid = std::this_thread::get_id();
        auto message = lambda_in();
        logevent event(source, level, when, tid, function, path, line, message);
        logengine::get_engine()->deliver(event);
    }
}

void mutex::lock() {
    std::mutex::lock();
    assert(owned_by == std::thread::id());
    owned_by = std::this_thread::get_id();
}

// THREAD this function needs to be proven to be thread safe
void mutex::unlock() {
    // FIXME is this always locked here? If so, why?
    assert(owned_by == std::this_thread::get_id());
    owned_by = std::thread::id();
    std::mutex::unlock();
}

// THREAD this function needs to be proven thread safe
bool mutex::caller_has_lock() {
    // FIXME is this thread safe? where is the locking?
    return owned_by == std::this_thread::get_id();
}

void shared_mutex::lock() {
    std::shared_timed_mutex::lock();
    assert(owned_exclusive_by == std::thread::id());
    owned_exclusive_by = std::this_thread::get_id();
}

// THREAD this function needs to be proven to be thread safe
void shared_mutex::unlock() {
    // FIXME is this always locked here? If so, why?
    assert(owned_exclusive_by == std::this_thread::get_id());
    owned_exclusive_by = std::thread::id();
    std::shared_timed_mutex::unlock();
}

void shared_mutex::lock_shared() {
    auto our_thread_id = std::this_thread::get_id();
    std::unique_lock<std::mutex> our_lock(lock_tracking_mutex);
    assert(shared_owners.find(our_thread_id) == shared_owners.end());
    auto result = shared_owners.insert(our_thread_id);
    if (! result.second) throw std::runtime_error("insert into set failed");
    std::shared_timed_mutex::lock_shared();
}

void shared_mutex::unlock_shared() {
    auto our_thread_id = std::this_thread::get_id();
    std::unique_lock<std::mutex> our_lock(lock_tracking_mutex);
    // erase() must run but if asserts are off the result is ignored
    UNUSED auto deleted_owners = shared_owners.erase(our_thread_id);
    assert(deleted_owners == 1);
    std::shared_timed_mutex::unlock_shared();
}

// THREAD this function needs to be proven thread safe
bool shared_mutex::caller_has_lockex() {
    // FIXME is this thread safe? where is the locking?
    return owned_exclusive_by == std::this_thread::get_id();
}

// returns true if at least a shared lock is held - also true if
// an exclusive lock is held
bool shared_mutex::caller_has_locksh() {
    if (caller_has_lockex()) return true;
    std::unique_lock<std::mutex> our_lock(lock_tracking_mutex);
    return shared_owners.find(std::this_thread::get_id()) != shared_owners.end();
}

bool lockable::caller_has_lock() {
    return lock_mutex.caller_has_lock();
}

lockable::lock lockable::get_lock() {
    return std::unique_lock<logjam::mutex>(lock_mutex);
}

shareable::write_lock shareable::get_lockex() {
    return std::unique_lock<mutex>(lock_mutex);
}

shareable::read_lock shareable::get_locksh() {
    return std::shared_lock<mutex>(lock_mutex);
}

bool shareable::caller_has_lockex() {
    return lock_mutex.caller_has_lockex();
}

bool shareable::caller_has_locksh() {
    return lock_mutex.caller_has_locksh();
}

// THREAD this function is thread safe
static bool logsource_compare(const char* rhs, const char* lhs) {
    assert(rhs != nullptr);
    assert(lhs != nullptr);

    // two pointers to the same string must match so check for that
    if (rhs == lhs) {
        return true;
    }

    // otherwise do a normal string compare
    return std::strcmp(rhs, lhs) == 0;
}

logsource::logsource(const char* c_str_in) : c_str(c_str_in) {
    assert(c_str != nullptr);
}

bool logsource::operator==(const char* rhs) const {
    assert(rhs != nullptr);
    return logsource_compare(c_str, rhs);
}

bool logsource::operator==(const logsource& rhs) const {
    assert(rhs.c_str != nullptr);
    return logsource_compare(c_str, rhs.c_str);
}

logevent::logevent(const std::string& source_in, const loglevel& level_in, const timestamp& when_in, const std::thread::id& tid_in, const char* function_in, const char *file_in, const int& line_in, const std::string& message_in)
: source(source_in), level(level_in), when(when_in), tid(tid_in), function(function_in), file(file_in), line(line_in), message(message_in)
{
    assert(level >= loglevel::unknown);

    if (function_in == nullptr) function_in = "void";
    if (file_in == nullptr) file_in = "(unknown)";
}

// THREAD this function is thread safe
logengine* logengine::get_engine() {
    static logengine engine;
    return &engine;
}

void logengine::add_destination(const std::shared_ptr<logdest>& destination_in) {
    auto lock = get_lockex();
    add_destination__lockex(destination_in);
}

// attempts to add a destination more than once silently return
// THREAD this function asserts required locking
void logengine::add_destination__lockex(const std::shared_ptr<logdest>& destination_in) {
    assert_lock(caller_has_lockex());

    for (auto&& i : destinations) {
        if (i->id == destination_in->id) {
            throw std::runtime_error("attempt to register duplicate log destination");
        }
    }

    destination_in->engine = this;
    destinations.push_back(destination_in);

    update_min_level__lockex();
}

void logengine::update_min_level() {
    auto lock = get_lockex();
    update_min_level__lockex();
}

loglevel logengine::get_min_level() {
    return min_log_level.load(std::memory_order_seq_cst);
}

// THREAD this function asserts required locking
loglevel logengine::set_min_level__lockex(loglevel level_in) {
    assert_lock(caller_has_lockex());

    auto known_level = get_min_level();
    if (known_level == level_in) {
        return known_level;
    }

    auto old_level = known_level;
    if (! min_log_level.compare_exchange_strong(known_level, level_in)) {
        // nothing but us should be writing to this
        throw std::runtime_error("compare and swap for logengine min log level failed");
    }

    return old_level;
}

// THREAD this function asserts required locking
void logengine::update_min_level__lockex() {
    assert_lock(caller_has_lockex());

    auto min_found = loglevel::fatal;

    for (auto&& i : destinations) {
        auto dest_level = i->get_min_level();
        if (dest_level < min_found) {
            min_found = dest_level;
        }
    }

    set_min_level__lockex(min_found);
}

// THREAD this function is inherently thread safe
bool logengine::should_log(const loglevel& level_in) {
    auto current_level = get_min_level();
    assert(current_level != loglevel::uninit);

    if (current_level == loglevel::none) return false;
    return level_in >= current_level;
}

void logengine::start() {
    auto lock = get_lockex();
    start__lockex();
}

// THREAD this function asserts required locking
void logengine::start__lockex() {
    assert_lock(caller_has_lockex());
    logevent* event_ptr;

    while(event_buffer.pop(event_ptr)) {
        deliver_to_all__locksh(*event_ptr);
        delete event_ptr;
    }

    started = true;
}

void logengine::deliver(const logevent& event_in) {
    auto lock = get_locksh();
    deliver__locksh(event_in);
}

// THREAD this function asserts required locking
// THREAD this function can use shared locking if the log
// event queue uses lockless insertion
// https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange
void logengine::deliver__locksh(const logevent& event_in) {
    assert_lock(caller_has_locksh());
    assert(event_in.level >= loglevel::unknown);

    // only deliver messages if started and then deliver them
    // even if that means 0 destinations receive them
    if (started) {
        deliver_to_all__locksh(event_in);
    } else if (buffer_events) {
        auto copied_event_ptr = new logevent(event_in);
        event_buffer.push(copied_event_ptr);
    }

    return;
}

void logengine::deliver_to_one__locksh(const std::shared_ptr<logdest>& dest_in, const logevent& event_in) {
    assert_lock(caller_has_locksh());
    if (event_in.level >= dest_in->get_min_level()) {
        dest_in->output(event_in);
    }
}

void logengine::deliver_to_all__locksh(const logevent& event_in) {
    assert_lock(caller_has_locksh());

    uint sent = 0;
    for(auto&& i : destinations) {
        deliver_to_one__locksh(i, event_in);
        sent++;
    }
    assert(sent == destinations.size());
}

logdest::logdest(const loglevel& min_level_in) : min_level(min_level_in) { }

// THREAD this function is inherently thread safe
logdest::destid logdest::next_destination_id() {
    static std::atomic<logdest::destid> last_dest_id = ATOMIC_VAR_INIT(0);
    return ++last_dest_id;
}

loglevel logdest::get_min_level() {
    return min_level.load(std::memory_order_seq_cst);
}

loglevel logdest::set_min_level(const loglevel& min_level_in) {
    // lock the entire engine while the log level of this destination
    // is changed - event delivery and buffering will block but
    // should_log() is still going to work with atomic operations
    // on the engine's min_log_level
    auto engine_lock = logengine::get_engine()->get_lockex();
    return set_min_level__lockreq(min_level_in);
}

// THREAD this function asserts correct locking
loglevel logdest::set_min_level__lockreq(const loglevel& min_level_in) {
    assert_lock(logengine::get_engine()->caller_has_lockex());

    loglevel old = get_min_level();
    min_level = min_level_in;

    if (engine != nullptr) {
        engine->update_min_level__lockex();
    }

    return old;
}

// THREAD this function is inherently thread safe
void logdest::output(const logevent& event_in) {
    assert(event_in.level >= get_min_level());
    handle_output(event_in);
}

// THREAD this function is thread safe
std::string logconsole::format_event(const logevent& event_in) const {
    return format_event_detailed(event_in);
}

// THREAD this function asserts the required locking
void logconsole::write_stdio__lockreq(const std::string& message_in) {
    // writing to stdio needs to be serialized so different threads don't overlap
    assert_lock(caller_has_lock());
    std::cout <<  message_in;
}

void logconsole::handle_output(const logevent& event_in) {
    // do the string work before the object becomes locked
    auto message = format_event(event_in);

    auto lock = get_lock();
    write_stdio__lockreq(message);
}

void logmemory::handle_output(const logevent& event_in)
{
    auto lock = get_lock();
    event_history.push_back(event_in);
    cleanup();
}

std::chrono::milliseconds logmemory::get_max_age() {
    auto lock = get_lock();
    return max_age;
}

void logmemory::set_max_age(std::chrono::milliseconds max_age_in) {
    auto lock = get_lock();
    max_age = max_age_in;
}

std::list<logevent> logmemory::get_event_history() {
    auto lock = get_lock();
    return event_history;
}

std::vector<std::string> logmemory::format_event_history()
{
    auto lock = get_lock();
    auto history_copy = event_history;
    lock.unlock();

    std::vector<std::string> retval;
    retval.reserve(history_copy.size());

    for (auto&& event : history_copy) {
        retval.push_back(format_event_detailed(event));
    }

    return retval;
}

void logmemory::cleanup() {
    auto now = std::chrono::system_clock::now();

    auto i = event_history.begin();

    while(1) {
        if (i == event_history.end()) {
            break;
        }

        auto current = i;
        i++;

        if (now - current->when >= max_age) {
            event_history.erase(current);
        } else {
            break;
        }
    }
}

}

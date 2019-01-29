Pulsar Audio Engine

This is an experimental signal processing system suitable for live
audio applications and construction of audio control systems. It is
an engine for mixing audio and applying audio effects made available
as a library or standalone headless application. Basically this is
the effects and mixing engine for a Digital Audio Workstation
(think Logic Pro X or Reaper) with out any GUI.

This software exists because I had an itch to scratch with my HAM
radio setup. I needed more and cheaper flexibility in signal processing
than I could put together with rack mount audio processing gear but
did not like any existing software for doing this.


Dependencies

This project requires cmake 3.8.2 but Debian/Stretch has only
3.7.2 available. You can get a compatible version from backports:

  apt-get install -t stretch-backports cmake

You'll need these packages to compile on Debian or Ubuntu

  cmake ladspa-sdk libboost-system-dev libjack-jackd2-dev
  libyaml-cpp-dev pkg-config libdbus-c++-dev libdbus-1-dev

You'll need these packages to use dev-config.yaml

  swh-plugins zam-plugins

It is possible the version of ZamPlugins in your distro is too old
to work at all. Also the quality of older plugins is not as good
as the most recent versions. It is best to install the most up to
date release from https://github.com/zamaudio/zam-plugins

To build the documentation you'll need

  doxygen


Building

Once all the dependencies are installed the project can be built
something like this:

  cmake . && make

*********************** THIS IS EXPERIMENTAL ***********************

That's not a joke. If this software is used, it dead locks, and
blows your speakers up or melts your amplifier it would just be
a tragic experiment. This software should not be used with any
audio equipment unless that equipment is ok being damaged.

Warnings aside the reliability is pretty good at this point with
the software routinely running for several days with out an issue
in the development enviornment.

Limitations

  * Only built and tested on Debian/GNU Linux and Ubuntu
  * No cycles allowed in the graph that defines the signal
    processing. They are not impossible but right now will
    cause a deadlock.


Implemented features

  * Multithreaded - effect plugins run in parallel if the topology allows for it
  * Configuration driven - YAML files define the signal processing chain and configuration
  * JACK audio client - participates in the whole JACK ecosystem
  * Load, configure and use any LADSPA plugin
  * Change effect configuration while audio engine is running.
  * Query and adjust plugin configuration via DBUS.


Planned features

  * Use as a library or standalone headless (no GUI) application
    * Use the library inside your application (threaded) or in another process (IPC)
    * Control the headless program via DBUS or other forms of IPC
  * Change the topology around while audio processing is running
  * Posix, Windows, and MacOS support
  * VST2 and VST3 on all supported platforms
  * Native Pulse Audio sources and sinks
  * Portaudio support as an alternative to JACK
  * Resampling for disparate clock domains (safe audio device aggregation)


Design

Pulsar is inspired heavily by the signal processing model used in GNU Radio. A
directed acyclic graph is constructed where the original audio comes in at the
roots of the forest and processed audio exits at the leafs. Each node on the
graph is a signal processing operation and the output of one node connects to
the input of another node. The members and topology of this network defines the
audio processing that will be done.

If the network topology includes any concurrent paths then they will be used
automatically if more than one audio thread is in use. The engine executes all
the nodes in dependency order so parallel operation is inherent. If concurrent
execution is at all possible it will happen with out any work from the user.
If concurrent operation is not possible there is still minimal overhead for
pure serial operation.

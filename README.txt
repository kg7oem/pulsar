Pulsar Audio Engine

This is a proof of concept signal processing system suitable for live
audio applications and construction of audio control systems. It is
an engine for mixing audio and applying audio effects made available
as a library or standalone headless application. Basically this is
the effects and mixing engine for a Digital Audio Workstation
(think Logic Pro X or Reaper) with out any GUI.

This software exists because I had an itch to scratch with my ham
radio station. I needed more and cheaper flexibility in signal processing
than I could put together with rack mount audio processing gear but
did not like any existing software for doing this.


Dependencies

This project requires cmake 3.8.2 but Debian/Stretch has only
3.7.2 available. This will generate warnings but it seems to
work anyway. You can get a fully compatible version from backports:

  apt-get install -t stretch-backports cmake

You'll need at least these packages to compile on Debian or Ubuntu

  cmake pkg-config libboost-system-dev libyaml-cpp-dev

The following features are conditional and require their
own packages:

  DBUS: libdbus-c++-dev libdbus-1-dev
  Portaudio: portaudio19-dev
  JACK Audio: libjack-jackd2-dev

You'll need these packages to use dev-config.yaml

  swh-plugins zam-plugins

It is possible the version of ZamPlugins in your distro is too old
to work at all. Also the quality of older plugins is not as good
as the most recent versions. It is best to install the most up to
date release from https://github.com/zamaudio/zam-plugins

To build the documentation you'll need

  doxygen graphviz


Building

Once all the dependencies are installed the project can be built
something like this:

  mkdir build
  cd build
  cmake ..
  make

The following compilers and OS configurations have been tested
and worked at least at one time:

  Debian Stretch GCC 6.3.0
  Ubuntu Bionic GCC 7.4.0 and Clang 6.0.0

The following compilers and OS configurations don't work

  Debian Stretch Clang 3.8.1


Status

This is experimental software undergoing major changes but basic
functionality is present. This software is also being used
as the audio control system in a live broadcast application
with my ham radio station and a shoutcast stream. For the
author reliability is good but there are limitations.

In its present form the software is considered to be research
for a prototype system. Once the features and architecture
are nailed down the system will be optimized into production
quality software.


Limitations

  * This thing is extremely brittle. Nearly all unexpected
    cases are fatal when they should be non-fatal errors.
  * Only built and tested on Debian/GNU Linux and Ubuntu.
  * No cycles allowed in the graph that defines the signal
    processing. They are not impossible but right now will
    cause a deadlock.


Implemented features

  * Multithreaded - effect plugins run in parallel if the topology allows for it
  * Configuration driven - YAML files define the signal processing chain and configuration
  * JACK audio client - participates in the whole JACK ecosystem
  * PortAudio client (default stream only currently)
  * Load, configure and use any LADSPA plugin
  * Change effect configuration while audio engine is running.
  * Query and adjust plugin configuration via DBUS.


Planned features

  * Use as a library or standalone headless (no GUI) application
    * Use the library inside your application (threaded) or in another process (IPC)
    * Control the headless program via DBUS or other forms of IPC
  * Change the topology around while audio processing is running
  * Posix, Windows, and MacOS support
  * LV2, VST2 and VST3 plugins on all supported platforms
    * Use plugins as filter nodes
    * Access Pulsar itself as a plugin in other applications
  * Native Pulse Audio sources and sinks
  * Resampling for disparate clock domains (safe audio device aggregation)
  * File IO for batch processing on the command line

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

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


*********************** THIS IS EXPERIMENTAL ***********************

That's not a joke. If this software is used, it dead locks, and
blows your speakers up or melts your amplifier it would just be
a tragic experiment. This software should not be used with any
audio equipment unless that equipment is ok being damaged.

Warnings aside the reliability is pretty good at this point with
the software routinely running for 12 hours with out an issue.

Limitations

  * Only built and tested on Debian/GNU Linux and Ubuntu
  * Not all connection forms work
    * Mixing from multiple outputs to one input
    * N:N connections from a node's output channels to another
      node's input channels


Implemented features

  * Multithreaded - effect plugins run in parallel if the topology allows for it
  * Configuration driven - YAML files define the signal processing chain and configuration
  * JACK audio client - participates in the whole JACK ecosystem
  * Load, configure and use any LADSPA plugin
  * Change effect configuration while audio processing is running


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

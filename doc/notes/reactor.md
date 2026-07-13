# Adding event-driven reactors in TSDuck

**Contents:**

* [Summary](#summary)
* [Reactor features](#reactor-features)
* [Current status](#current-status)
  * [Core Reactor](#core-reactor)
  * [Socket layer, including TLS](#socket-layer-including-tls)
  * [Presentation layer (Telnet, TLV)](#presentation-layer-telnet-tlv)
  * [Generic server](#generic-server)
  * [Worker delegation](#worker-delegation)
  * [Remaining work](#remaining-work)

## Summary

TSDuck uses a classical multi-threaded blocking-I/O architecture. There are
several reasons for this:

- Legacy code, as usual.

- Isolation between `tsp` plugins. Note that this is "execution isolation"
  only, avoiding that a plugin using blocking-I/O blocks other plugins. There
  is no memory or security isolation, as usual in multi-threaded environments.

- CPU load balancing between plugins. Some plugins can be CPU-intensive, such
  as encryption plugins. We need to leverage multi-core architectures.

- Multiple libraries which encapsulate blocking-I/O and cannot be used in an
  event dispatcher (SRT, RIST for instance).

- Multiple operating systems support. Windows, Linux, macOS, BSD have very
  different ways of handling asynchronous I/O in an efficient way. It is not
  easy to unify them into one single portable API.

However, this multi-threaded architecture creates additional problems in
plugins which act as network servers, for instance, in addition to the standard
packet processing features. In that case, the plugin is obliged to create one
or more internal additional threads.

It would be nice to have some "reactor" feature in TSDuck. We call reactor what
is also known as "event dispatcher", "event loop", etc. The reactor pattern is
not incompatible with a multi-threaded architecture. TSDuck would use an hybrid
architecture where several threads coexist when independence of execution is
required (in plugins for instance). Additionally, inside each thread, there is
possibly one reactor which dispatches events from distinct sources which are
closely linked to the thread or plugin, provided that asynchronous I/O is
possible in a portable way in that context.

## Reactor features

An event loop shall include the following features:

- One-shot simple timer: Invoke one callback when a timer expires. The callback
  is specified when the timer is armed. A timer can be relative (a duration
  from now), or absolute (a date/time).

- General timer: Invoke callbacks when a timer expires. A timer is a C++
  object. Several objects can subscribe to that timer object. A general timer
  can be one-shot or repeated. A timer can be relative (a duration from now),
  or absolute (a date/time).

- User event: Invoke callbacks when a user signals the event. An event is a C++
  object. Several objects can subscribe to that event object.

- Message queue: Invoke callbacks when a message is available in the queue. The
  callback is then responsible for removing messages from the queue, until it
  becomes empty.

- Asynchronous I/O, depending on feasibility on each OS or library. Specify one
  callback when starting the I/O operation.
  - Raw UDP and TCP.
  - Asynchronous TLS.
  - HTTP session.
  - TS files.
  - Text files.
  - Generic files.

- Cancellation: Cancel pending due events: timers, asynchronous I/O. Question:
  shall we invoke a specific callback to notify the class which expected the
  callback?

- Worker delegation: When a callback in the reactor needs to run some lengthy
  computing, or call a library with blocking I/O, it cannot do it sequentially
  because it would block the reactor. Instead, it should delegate that
  treatment to another work thread. A completion callback will be called in the
  reactor when the delegated task completes. There is a pool of a maximum
  number of worker threads. A delegated task is either 1) executed in an idle
  worker thread, 2) executed in a newly created worker thread if none were
  idle, 3) queued for later execution if the maximum number of worker threads
  is reached.

## Current status

### Core Reactor

Class `Reactor` is implemented. Based on epoll (Linux), kqueue (macOS and BSD), I/O
Completion Ports (Windows). Timers and user events are included. Non-blocking I/O
(epoll, kqueue) and asynchronous I/O (IOCP) are implemented using distinct API's.

Timers and user events are directly usable by applications. I/O management should
be reserved to specialized "reactive" classes.

We differentiate the class `Reactor` which is the core class for event dispatching
and "reactive" classes. The latter are specialized classes which use a common
instance of `Reactor` to dispatch events.

`Reactor` and reactive classes never block. They only implement services to "start
something". When that "something" completes, a handler interface classes is called.

### Socket layer, including TLS

UDP and TCP sockets are implemented in separate classes. Internally, they use
distinct code paths for non-blocking and asynchronous I/O. TCP client and server
are implemented. TLS subclasses encapsulate TLS 1.2 and 1.3.

### Presentation layer (Telnet, TLV)

Distinct classes implement data encoding and decoding in various formats. Current
formats are text lines (Telnet protocol) and TLV (tag-length-value, as used in
DVB SimulCrypt protocols).

The presentation classes `ReactiveTelnetConnection` and `ReactiveTLVConnection`
need an associated instance of a reactive TCP class. An association relationship
was preferred to inheritance to allow any subclass of `ReactiveTCPConnection`.
Thus, a line-based or TLV connection can be transparently implemented over a
clear TCP connection as well as over a TLS tunnel.

### Generic server

Class `ReactiveServer` implements the logic of a generic TCP server. It manages
the connections of incoming clients and the creation of "client sessions", one
per client connection.

Using a user-supplied factory class, any kind of transport (clear TCP or TLS)
and any kind of data presentation (raw, text lines, TLV) can be used.

### Message queues

Template class `ReactiveMessageQueue` is a wrapper around an instance of template
class `MessageQueue` which receives the messages in a reactor context.

### Worker delegation

Class `ReactiveWorkerPool` implements delegation of lengthy or blocking tasks to
a pool of threads. It uses two distinct handler interface classes. One is used
in the context of a worker thread to perform the lengthy task. The other one
is used in the context of the reactor thread to notify the application of the
completion of the delegated task.

### Remaining work

- HTTP session (libcurl, WinInet).
- Pipes.
- Generic files.

# Adding event-driven reactors in TSDuck

**Contents:**

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

- Asynchronous I/O, depending on feasability on each OS or library. Specify one
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

## Implementations

### Linux

Investigate `io_uring`. Is it possible to wake up on timers or user-triggered
events?

See https://kernel.dk/io_uring.pdf

When using `poll()`, the max number of file descriptors is `RLIMIT_NOFILE`, a
dynamic value. By default, `ulimit -n` reports 1024. The max allowed value to
set is in `/proc/sys/fs/file-max`, which is typically 2^63.

### macOS and BSD

When using `poll()`, the max number of file descriptors is `OPEN_MAX`.

On macOS, `OPEN_MAX` is 10240, so not a real limitation for `poll()`.

On FreeBSD, `OPEN_MAX` is 64, definitely a limitation. However, the man page
for `poll()` does not mention `OPEN_MAX`. It mentions the "system tunable
`kern.maxfilesperproc` and `FD_SETSIZE`". `sysctl kern.maxfilesperproc` reports
234945 and `FD_SETSIZE` is defined to 1024. So, less a limitation.

`OPEN_MAX` is defined in `sys/syslimits.h` and `FD_SETSIZE` is defined in
`sys/select.h`.

### Windows

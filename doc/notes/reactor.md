# Adding event-driven reactors in TSDuck

**Contents:**

## Summary

TSDuck uses a classical multi-threaded blocking-I/O architecture. There are
several reasons for this:

- Legacy code, as usual.
- Execution isolation between `tsp` plugins.
- CPU load balancing between plugins. Some plugins can be CPU-intensive, such
  as encryption plugins. We need to leverage multi-core architectures.
- Multiple libraries which encapsulate blocking-I/O and cannot be used in an
  event dispatcher (SRT, RIST for instance).
- Multiple operating systems support. Windows, Linux, macOS, BSD have very
  different ways of handling asynchronous I/O in an efficient way. It is not
  easy to unify them into one single portable API.

However, this multi-threaded architecture creates additional problems in
plugins which act as network servers, for instance, in addition to the standard
packet processing features.

It would be nice to have some "reactor" feature in TSDuck. We call reactor what
is also known as "event dispatcher", "event loop", etc. The reactor pattern is
not incompatible with a multi-threaded architecture. TSDuck would use an hybrid
architecture where several threads coexist when independence of execution is
required (in plugins for instance). Additionally, inside each thread, there is
possibly one reactor which dispatches events from distinct sources which are
closely linked to the thread or plugin.

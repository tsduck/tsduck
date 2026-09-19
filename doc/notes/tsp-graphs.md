# Project to add graphs in tsp

**Contents:**

* [Summary](#summary)
* [Graphs of plugins and dispatch points](#graphs-of-plugins-and-dispatch-points)
* [Command line syntax](#command-line-syntax)
  * [Specification of dispatch points](#specification-of-dispatch-points)
  * [Syntax rules](#syntax-rules)
  * [Local and global options for dispatch points](#local-and-global-options-for-dispatch-points)
  * [Required checks](#required-checks)
* [Remote control](#remote-control)
* [Input features of a dispatch point](#input-features-of-a-dispatch-point)
  * [Input numbering](#input-numbering)
  * [Input cycles removal](#input-cycles-removal)
  * [Input modes](#input-modes)
  * [Event notification](#event-notification)
* [Output feature of a dispatch point](#output-feature-of-a-dispatch-point)
* [Termination conditions](#termination-conditions)
  * [End-of-file propagation](#end-of-file-propagation)
  * [Abort back-propagation](#abort-back-propagation)
  * [Glocal termination](#glocal-termination)
* [Deprecated command and plugins](#deprecated-command-and-plugins)
  * [Removal of command tsswitch](#removal-of-command-tsswitch)
  * [Plugins fork and merge](#plugins-fork-and-merge)
  * [Plugins file, ip](#plugins-file-ip)
* [Open topics](#open-topics)
* [Implementation planning](#implementation-planning)

## Summary

With TSDuck version 3, the command `tsp` implements a linear chain of plugins,
from one input plugin to one output plugin. The chain is fully synchronous,
without packet loss, blocking when necessary.

The first idea is to allow several input plugins by moving most features of
`tsswitch` into `tsp`. The command `tsswitch` would then be removed.

The second idea is to allow split points where the chain of plugins is forked
between two or more chains. This is similar to a packet processing plugin `fork`
running another instance of `tsp`, but staying in the same process.

These two concepts were already possible to implement using `tsswitch` and
plugins `fork`. The benefits of their integration in `tsp` are:

- Improved performance and latency. Avoid context switches between
  processes. Avoid the kernel buffering of packets using system pipes, as well
  as associated latency.

- More consistent command lines. Using several nested levels of plugins `fork`
  in a script is hard and error-prone. The only reliable way of achieving this
  is using multiple [partial command line
  redirection](https://tsduck.io/docs/tsduck.html#cmd-redirection) using `@`.

This change is fundamental. Some commands and plugins will become obsolete. Some
command lines will have non-backward compatible changes. Therefore, we consider
this as a major version: TSDuck version 4.

## Graphs of plugins and dispatch points

A dispatch point receives packets from one or more plugins and passes these
packets to one or more plugins.

A dispatch point is identified by a unique name. The name is an arbitrary string
which is defined by the user, on the command line.

The following `tsp` graph contains two dispatch points, named "a" and "b".

~~~
                             +===+
+--+     +---+   +-------+   |...|   +-----+   +----+
|ip|---->|zap|-->|analyze|-->X...X-->|count|-->|file|
+--+     +---+   +-------+   |. .|   +-----+   +----+
                             | a |
                             |. .|              +===+
+----+   +---+   +-------+   |...|   +------+   |...|   +---+   +---+
|http|-->|zap|-->|analyze|-->X...X-->|tables|-->X...X-->|sdt|-->|srt|
+----+   +---+   +-------+   |...|   +------+   |. .|   +---+   +---+
                             +===+              | b |
                                                |. .|   +---+   +----+
                                                |...X-->|pmt|-->|rist|
                                                |...|   +---+   +----+
                                                +===+
~~~

Currently, a `tsp` plugin chain is linear. It starts with an input plugin and
ends in an output plugin.

In the proposed architecture, `tsp` now uses a graph of plugins. A graph is
divided in several "branches". A branch is a linear chain of plugins which
works the same way as the current version of `tsp`. A branch starts with either
an input plugin or an output of a dispatch point. A branch ends in either an
output plugin or an input of a dispatch point.

A branch has a global packet buffer, as in `tsp` version 3. In the presence of
dispatch points, an instance of `tsp` has several global buffers, one per
branch. A dispatch point is responsible from moving TS packets from one global
buffer to another.

Internally, a dispatch point may be split in two distinct entities, the input
switching/mixing part and the output spliting/duplicating part. Each part may
have its own switching/mixing and spliting/duplicating strategy.

## Command line syntax

Because a command line is linear by definition, it is difficult to represent a
graph.  Instead, a `tsp` command line is made of a list of branches. Each
branch has an identified beginning (an input plugin or an output of a dispatch
point) and an identified end (an output plugin or an input of a dispatch
point). Therefore, there is no ambiguity in the identification of a branch.

### Specification of dispatch points

Plugins are still identified by the usual options `-I`, `-P`, `-O`. The
following options are added to identify inputs and outputs of a dispatch point:

- `-B name [options]`: Output of a dispatch point, [B]eginning of a branch.
- `-E name [options]`: Input of a dispatch point, [E]nd of a branch.

In both cases, "name" is the given name of the dispatch point. Each time an
option `-B a` or `-E a` is found, the option defines an output or input of the
same dispatch point named "a".

Therefore, the diagram above is run using the following command. For clarity,
the options of the various plugins were omitted.

~~~
tsp -I ip -P zap -P analyze -E a \
    -I http -P zap -P analyze -E a \
    -B a -P count -O file \
    -B a -P tables -E b \
    -B b -P sdt -O srt \
    -B b -P pmt -O rist
~~~

Inside each branch, the order of plugins is significant. However, the branches
may be specified in any order. The graph will be built from the specification
of the various dispatch points, based on their name.

### Syntax rules

The following rules apply to the command line structure:

- A branch always starts with `-I` or `-B`.

- A branch always ends with `-O` or `-E`.

- As a consequence of the previous rules, it is no longer possible to place the
  options `-I` and `-O` anywhere on the command line, as it was possible with
  TSDuck version 3, when `tsp` had one single chain of plugins.

- If the first plugin in the command line is neither `-I` nor `-B`, it defaults
  to `-I file`, meaning reading on the standard input. This is compatible with
  the defaults of `tsp` version 3.

- Similarly, if the last plugin in the command line is neither `-O` nor `-E`,
  it defaults to `-O file`, meaning writing to the standard output.

- A dispatch point is defined by all options `-E` and `-B` with the same name.

- A dispatch point must have at least one `-E` and one `-B`.

- A dispatch point with exactly one `-E` and one `-B` is "optimized away": it
  is removed and the two sides of the dispatch point are merged in the same
  branch and same global packet buffer. There are exceptions with some specific
  options such as `-B name --lossy` where the dispatch point has a specific
  behavior which cannot be removed.

### Local and global options for dispatch points

The specification of inputs and outputs of a dispatch point may have options,
as in `-E name [options]` and `-B name [options]`.

There are two types of options for `-E` and `-B`:

- Local options: Some options are specific to a given input or output of a
  dispatch point. Examples include `-E name --primary` or `-B name --lossy`.
  These options may be specified or omitted in each reference to the dispatch
  point, because they apply to a given input or output of the dispatch point.

- Global options: Some options describe the global behavior of input or output
  of the dispatch point. Examples include `-E name --live` or `-E name
  --receive-timeout`.

  - To avoid confusion, all these options must be grouped into the same
    occurrence of the dispatch point on the command line. This may not be the
    first occurrence, but all global options for a given dispatch point must be
    specified in the same occurrence.
  
  - All global input options must appear in the same occurrence of `-E name`
    for a given "name".

  - All global output options must appear in the same occurrence of `-B name`
    for a given "name".

### Required checks

The TS processor shall perform the following checks:

- No loop: Starting from each input plugin (`-I`), walk through all possible
  output paths of each dispatch point and check that each plugin is visited at
  most once. Stop with an error when reaching a plugin which has already been
  visited, starting from the same input plugin.

- No partitioning: Starting from the first input plugin only, walk through all
  possible _input and output_ paths of each dispatch point and check that all
  plugin have been visited.

## Remote control

For the sake of security, clarity, and consistency, the `tsp` remote control is
performed using the Web API only. The simple line-oriented TCP protocol,
similar to a Telnet session, is removed. The command `tspcontrol` becomes the
only way to control a remote `tsp` session (although it is possible to emulate
its behavior using complex `curl` commands).

By default, the communication is encrypted using TLS. Using option
`--control-no-tls` is possible but not recommended. This should be limited to
specific builds without OpenSSL, for constrained environments.

By default, when no certificate is provided, neither on the command line nor
using environment variables, a certificate is generated using an ephemeral
3072-bit RSA key. This means that `tspcontrol` must be used with option
`--insecure`.

The command `tsswitch` being removed, the remote input switching capabilities
are merged into the command `tcpcontrol`. The simple and insecure UDP mechanism
is removed.

## Input features of a dispatch point

### Input numbering

As with `tsswitch`, each input of a dispatch point is numbered, from 0 to N-1,
based on the ordering on the command line. These numbers are used to remotely
switch inputs, for instance.

Note that this number is the input number of the dispatch point. This is not the
plugin number. The numbering of all plugins on the command line remains an open
point.

### Input cycles removal

The command `tsswitch` used to implement "cycles". All inputs were read in
sequence the specified number of times (one by default) or infinitely.

This was possible because `tsswitch` directly worked on input plugins and it is
possible to restart an input plugin. In the case of graphs, the input of a
dispatch point can be any chain of plugins, including upstream dispatch
points. "Restarting" an input branch is not defined.

Therefore, specifying a number of cycles or any feature that implies restarting
an input plugin are removed.

### Input modes

Several input modes are defined:

- Sequential (the default): When an input branch is active, block all other
  input branches. No packet is lost on other branches, they will be delivered
  when the dispatch point switches to these input branches.

- Live (global option `--live`): Packets coming from other input branches are
  dropped. No input branch is blocked but packets are lost. This mode is
  typically used when all inputs are live streams on distinct sources.

- Mix (global option `--mix`): All incoming packets are passed to output
  branches, from all input branches, at the time they arrive. There is no
  guarantee on the order of packets between distinct input branches or bursts of
  packets coming from the same input. We call it "mix" and not "mux" because the
  result is an unmanaged mixture of packets, not proper multiplexing.

Associated options:

- Global option `--receive-timeout value`: When no packet is received from the
  current input branch within the specified number of milliseconds, switch to
  the next input branch.

- Local option `--first`: Specify that this input branch should be used
  first. By default, the first input branch on the command line is used.

- Local option `--primary`: Specify the input branch which is considered as
  primary or preferred. Always try to read from that branch. If no packet is
  present during the specified receive timeout, switch to another input
  branch. However, whenever packets reappear on the primary branch,
  automatically switch back to it.

### Event notification

All options `--event-*` from `tsswitch` are retained as global options for
dispatch point inputs.

## Output feature of a dispatch point

At a dispatch point, TS packets are duplicated and copied in all output chains.
A packet is copied from the input chain buffer into all output chains buffers at
the same time.

We define several types of output for dispatch points. The type of output is set
in each `-B` option because it is specific to that output (they are local
options). A dispatch point may have outputs of distinct types.

- Synchronous output (the default): Packets are synchronously transmitted,
  without loss, blocking when necessary. As a consequence, if that output branch
  is blocked downstream and its buffer is full, input of the dispatch point as
  well as all other output branches are blocked at the dispatch point until the
  output branch is unblocked.
  
- Lossy output (local option `--lossy`): Packets are transmitted when possible
  only. If that output branch is blocked downstream and its buffer is full, the
  incoming packets are missed for that branch but the input chains and the
  other outputs of the dispatch point are not blocked.

- Deadly output (local option `--deadly`): Packets are transmitted when
  possible only and the transmission stops when the first packet is dropped. In
  that case, an end-of-file is transmitted downstream to that output branch. No
  more packet will be passed to that output. The branch is considered as dead.

## Termination conditions

### End-of-file propagation

An EOF condition is propagated downstream, from plugin to plugin.

When an EOF reaches the input of a dispatch point, this input is considered as
completed and another input is selected. When all inputs of a dispatch point are
completed, an EOF condition is propagated to all output branches which are not
already dead.

### Abort back-propagation

In TSDuck version 3, when an intermediate or output plugin reports a fatal
error, an "abort" signal is propagated backward to all plugins to stop them.

In the new version of `tsp` if an abort is propagated from a chain backward to
the output of a dispatch point, the dispatch point considers that output as
"dead" (as with "deadly" outputs). No further packet will be sent to that
output.

When all outputs of a dispatch point are dead, an abort is transmitted backward
to all input branches of that dispatch point, regardless of the reason for the
dead output (abort from downstream or not possible to pass a packet to a
"deadly" output).

### Glocal termination

A dispatch point is considered as terminated or closed when it has sent either
an EOF condition downstream, or an abort condition upstream, to all branches.

The TS processor is fully terminated when all plugins and dispatch points are
closed.

## Deprecated command and plugins

### Removal of command tsswitch

The command `tsswitch` becomes obsolete and will be removed from TSDuck version 4.

A legacy `tsswitch` is typically replaced with the following graph:

~~~
         +=====+
+--+     |.....|
|ip|---->X.....|
+--+     |.....|
         |.....|
+----+   |.. ..|   +---+
|http|-->X. a .X-->|srt|
+----+   |.. ..|   +---+
         |.....|
+----+   |.....|
|rist|-->X.....|
+----+   |.....|
         +=====+
~~~

### Plugins fork and merge

The packet processing plugins `fork` and `merge` are used to duplicate the TS
or merge it with another one, using an external command. With TSDuck version
3, the only way to create graphs is using these plugins on other `tsp`
commands. With TSDuck version 4, this type of usage is deprecated since complex
graphs can be built inside one single instance of `tsp`.

Note that `merge` is not strictly equivalent to a dispatch point because it
replaces stuffing packets only.

The plugins `fork` and `merge` remain useful to route TS to and from other
commands.

### Plugins file, ip

The plugins `file` and `ip` exist in three forms: input plugin, output plugin
and packet processing plugin. With TSDuck version 4, the packet processing
versions are no longer necessary because the feature can be implemented as a
dispatch point and the corresponding output plugin.

However, to avoid rewriting more complex command line, and because these
plugins are relatively simple, these plugins will remain unchanged.

## Open topics

- Suspend / restart issue: When a plugin is suspended, packets are directly
  passed to the next plugin, without going through the suspended plugin. What is
  the impact when the suspended plugin is just before a dispatch point?

- Plugin numbering: Currently, plugins are sequentially numbered from 0 (input)
  to N-1 (output). How to manage plugin numbers after across dispatch points?

## Implementation planning

As usual, there is no planning. TSDuck development is based on good will, spare
time and unpaid work.

# Project to add graphs in tsp

**Contents:**

* [Summary](#summary)
* [Multiple inputs (tsswitch integration)](#multiple-inputs-tsswitch-integration)
  * [Command line syntax](#command-line-syntax)
    * [New options in tsp (copied from tsswitch)](#new-options-in-tsp-copied-from-tsswitch)
    * [Incompatibilities with tsswitch command line](#incompatibilities-with-tsswitch-command-line)
  * [Input buffer management](#input-buffer-management)
* [Split points in the chain of plugins](#split-points-in-the-chain-of-plugins)
  * [Nature of a split point](#nature-of-a-split-point)
  * [Command line syntax](#command-line-syntax)
  * [Miscellaneous topics](#miscellaneous-topics)
* [Implementation planning](#implementation-planning)

## Summary

Originally, the command `tsp` implements a linear chain of plugins, from one
input plugin to one output plugin. The chain is fully synchronous, without
packet loss, blocking when necessary.

The first idea is to allow several input plugins by moving all features of
`tsswitch` into `tsp`. The command `tsswitch` would then be deprecated (or
aliased to `tsp`).

The second idea is to allow split points where the chain of plugins is forked
between two chains. This is similar to a packet processing plugin `fork`
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

## Multiple inputs (tsswitch integration)

### Command line syntax

Most command line options from `tsswitch` are added to `tsp`. In case of
conflict, the priority is to keep the compatibility with the existing syntax of
`tsp`, assuming that there are more existing `tsp` commands in use.

The main difference if that `tsp` will accept multiple options `-I`.

#### New options in tsp (copied from tsswitch)

- `--allow ip-address`
- `-c value` and `--cycle value`
- `--delayed-switch`
- `--event-command 'command'`
- `--event-local-address ip-address`
- `--event-ttl value`
- `--event-udp ip-address:port`
- `--event-user-data 'string'`
- `--fast-switch`
- `--first-input value`
- `--infinite`
- `--no-reuse-port` (maybe useless)
- `-p value` and `--primary-input value`
- `--remote [ip-address:]port`
- `--remote-certificate-path name`
- `--remote-key-path name`
- `--remote-store name`
- `--remote-tls`
- `--remote-token string`
- `--terminate`
- `--udp-buffer-size value`

Note that `tsp` and `tsswitch` already share several options, with the same
semantics.

Open point: the "remote" feature of `tsswitch` may be merged with the "control"
features of `tsp`.  The options `--remote-*` would then be removed. The issue
is that the "remote" feature uses a different communication protocol, which
would create incompatibilities in existing configurations.

#### Incompatibilities with tsswitch command line

- Option `--receive-timeout` in `tsswitch` is replaced with `--input-timeout`
  in the new `tsp` because `--receive-timeout` is already used in `tsp` with a
  different semantics.

- `-a` no longer accepted for `--allow`, it is already assigned to
  `--add-input-stuffing`.

- `-b value` and `--buffer-packets value` no longer accepted. Use the `tsp`
  option `--buffer-size-mb` to set the size of all packet buffers. Also applies
  to forked branches of plugins (see split points).

- `-d` no longer accepted for `--delayed-switch`, it is already assigned to
  `--debug` in `tsp`. In theory, the generic option `--debug` grabs option `-d`
  only when no other command-specific option uses it. However, we consider that
  many existing `tsp` commands may already use `-d` for `--debug`.

- `-f` no longer accepted for `--fast-switch`. Would be too specific since
  other input modes are no longer abbreviated.

- `-i` no longer accepted for `--infinite`, it is already assigned to
  `--ignore-joint-termination`.

- `-r` no longer accepted for `--remote`, it is already assigned to
  `--realtime`.

- `-t` no longer accepted for `--terminate`, it is already assigned to
  `--timed-log`.

### Input buffer management

In `tsswitch`, each input plugin has it own packet buffer. Packets are then
copied into the output packet buffer. In `tsp`, there is one global packet
buffer which is used by all plugins.

Possible solution: Use the global packet buffer when there is only one input
plugin and individual buffers when there are more than one input plugin. Pros:
performance in the general case (one input plugin). Cons: more complex.

## Split points in the chain of plugins

### Nature of a split point

At a split point, TS packets are duplicated and copied in two chains of
plugins. In practice, there are not two new chains. There is one "main" chain
of plugins, as before. At the split point, a new chain of plugins is created
and packets are duplicated into the new chain. Let's call it the "forked"
chain.

Because chains of plugins can be split, let's call them "branches". At a split
point, there are two branches. The "main branch" continues its normal flow. The
"forked branch" starts at the split point.

The main branch has one global packet buffer, as previously. The split point is
located between two plugins and packets are copied out of the global buffer of
the main branch at that point. The forked branch has its own global packet
buffer. The packets are introduced in that new global buffer at the split
point, as if it was an input plugin of the forked branch.

A packet is copied from the main buffer into the forked buffer at the same time
that packet is passed to the next plugin after the split point in the main
branch.

In the main branch, packets flow as usual, synchronously, without loss. If the
main branch is blocked downstream and the main buffer is full, the main branch
is blocked and packets cannot be passed to the next plugin. As a consequence,
packets cannot be copied either in the forked branch, even if the forked branch
is ready to process packets.

We define two types of split points:

- Synchronous split points.
- Lossy (or loose) split points.

Packet transmission strategies:

- Synchronous split point: Packets are duplicated in the forked branch with the
  same strategy as the main branch: synchronously, without loss, blocking when
  necessary. As a consequence, if the forked branch is blocked downstream and
  its buffer is full, the main branch is blocked at the split point until the
  forked branch is unblocked.

- Lossy split point: If the forked branch is blocked downstream and its buffer
  is full, a packet is immediately passed to the next plugin in the main branch
  and the packet is lost for the forked branch.

Abort reverse transmission strategies (after an abort somewhere in the forked
branch is transmitted back to the first plugin of the foked branch):

- Synchronous split point: In the main branch, an abort is transmitted backward
  and an "end of input" is transmitted downstream, as if the abort occurred in
  a plugin at the split point.

- Lossy split point: Nothing changes in the main branch. The forked branch is
  declared as "dead" and no packet will be transmitted in the forked
  branch. The split point is considered as no longer existent.

### Command line syntax

The command line is linear by design and it is not easy to express the concept
of split point. We do it in two steps. First, we identify split points inside
the main branch. A name is assigned to each split point (it can be as simple as
`1`, `2`, or `a`, `b`). Later, after the end of the main branch, the named
forked branch is fully detailed with its list of plugins.

The split points and full branches of plugins are identified by one-letter
uppercase options, like `-I`, `-P`, and `-O` for plugins in a branch:

- `-S name`: Identify the location of a synchronous split point with the given
  `name`.

- `-L name`: Identify the location of a lossy split point with the given
  `name`.

- `-B name`: Start the description of the branch starting at the split point
  which is identified by the given `name`.

Example (omitting the options in the plugins):

~~~
tsp -I ip -P filter -P analyze -S a -P count -O file \
    -B a -P pcrbitrate -P tables -L b -P zap -O srt \
    -B b -P cat -P pmt -O rist
~~~

The main branch of plugins starts with input plugin `ip` and ends with output
plugin `file`. In the middle, there is a synchronous split point named `a`. The
branch `a` starts at this split point and ends with output plugin `srt`. In the
middle of branch `a`, there is a lossy split point named `b`. The branch `b`
starts at this split point and ends with output plugin `rist`.

~~~
+--+   +------+   +-------+      +-----+   +----+
|ip|-->|filter|-->|analyze|--a-->|count|-->|file|
+--+   +------+   +-------+  |   +-----+   +----+
                             |
                             |   +----------+   +------+      +---+   +---+
            branch a:        +-->|pcrbitrate|-->|tables|--b-->|zap|-->|srt|
                                 +----------+   +------+  |   +---+   +---+
                                                          |
                                                          |   +---+   +---+   +----+
                                           branch b:      +-->|cat|-->|pmt|-->|rist|
                                                              +---+   +---+   +----+
~~~

### Miscellaneous topics

- Suspend / restart issue: When a plugin is suspended, packets are directly
  passed to the next plugin, without going through the suspended plugin. What
  is the impact when the suspended plugin is just before a split point?

- Plugin numbering: Currently, plugins are sequentially numbered from 0 (input)
  to N-1 (output). How to manage plugin numbers after split points? Solution 1:
  restart at N to first branch, etc. Solution 2: Structured numbering which
  branch names, eg `a.0`, `a.1`, etc. Solution 1 is probably easier to
  implement.

## Implementation planning

As usual, there is no planning. TSDuck development is based on good will, spare
time and unpaid work.

Split points will probably be implemented first, then multiple inputs.

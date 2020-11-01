# Python bindings  {#pythonbindings}
[TOC]

# Overview  {#pyoverview}

Starting with version 3.24, TSDuck includes Python bindings to some
high-level features.

Although subject to evolution, these bindings will never aim at
supporting the full TSDuck feature set since this would be too large.
Only a small subset of TSDuck high-level features are targeted.

It is currently in experimental stage only and subject to radical change
in the near future. The current code is probably far from the "pythonic way"
but is willing to evolve toward it.

## Python prerequisites  {#pyprereq}

The code was initially tested with Python 3.7 and higher.
Python 2.x is not supported.
Intermediate versions may work but without guarantee.

## Implementation notes  {#pyimplem}

There are usually two ways to call C/C++ from Python:

- Using the predefined `ctypes` Python module to call C functions,
- Implementating a full native Python module in C/C++.

The second option is usually more flexible and more generic. However,
the generated binary depends on the version of Python. If such an option
is used, the binary installation of TSDuck would require a specific version
of Python (or a specific set of versions of it). But each system has it own
requirements on Python and it is difficult for a product like TSDuck to
impose a specific version of Python.

Consequently, the less flexible `ctypes` approach was chosen. The TSDuck
binary library contains C++ wrapper functions to some features of TSDuck and
these carefully crafted functions are directly called from Python code
using `ctypes`, regardless of the version of Python. Note, however, that
these C++ are hidden inside the Python bindings and invisible to the
application developer.

# Using TSDuck Python bindings  {#pyusing}

All TSDuck bindings are defined in a module named `ts`.
All Python programs using TSDuck shall consequently start with:
~~~
import ts
~~~

## Linux  {#pylinux}

The Python bindings are installed with TSDuck in `/usr/share/tsduck/python`.
Simply add this directory in the environment variable `PYTHONPATH` to use
TSDuck from any Python application:

~~~
export PYTHONPATH="/usr/share/tsduck/python:$PYTHONPATH"
~~~

## macOS  {#pymac}

This is similar to Linux, except that `/usr/local/share` is used instead of `/usr/share`.

~~~
export PYTHONPATH="/usr/local/share/tsduck/python:$PYTHONPATH"
~~~

## Windows  {#pywin}

On Windows, Python bindings are optional components of the TSDuck installer.
When they are selected for installation, they are installed in the TSDuck
area and the environment variable `PYTHONPATH` is modified at system level
to include the root directory of the TSDuck Python bindings. Thus, any
Python program can use TSDuck directly.

# TSDuck Python bindings reference  {#pyref}

All TSDuck bindings are defined in a module named `ts`, the same name as
the C++ namespace for the TSDuck library.

## Informational functions  {#pyinfofunc}

The function `ts.version()` returns the TSDuck version as a string.

The function `ts.intVersion()` returns the TSDuck version as an integer,
containing the major, minor and commit number. This value is suitable
for comparison.

Example:
~~~
$ python3
>>> import ts
>>> ts.version()
'3.24-2033'
>>> ts.intVersion()
32402033
>>> 
~~~

## Reporting classes  {#pyreport}

All TSDuck developers know that message reporting is performed throughout the
TSDuck library using `ts::Report` objects. The C++ abstract class `ts::Report`
has many different subclasses for various usages.

The reporting features are not really essential to Python users but having created
such objects is required to use most TSDuck features, including from Python.
Consequently, a Python class `ts.Report` and a few subclasses are available to
Python developers.

The following table lists the various Python subclasses and their C++ equivalent.

| Python class | C++ equivalence
| ------------ | ---------------
| `ts.Report`       | `ts::Report`
| `ts.NullReport`   | `ts::NullReport`
| `ts.StdErrReport` | `ts::CerrReport`
| `ts.AsyncReport`  | `ts::AsyncReport`

The class `ts.AsyncReport` is especially useful to instantiate the class `ts.TSProcessor`.

The basic behaviour of `ts.Report` is the same as the C++ reporting classes.
The severity levels exist in Python as listed in the table below.
The default severity level of a `ts.Report` object is `ts.Report.INFO`.

| Python value | C++ equivalence
| ------------ | ---------------
| `ts.Report.FATAL`   | `ts::Severity::Fatal`
| `ts.Report.SEVERE`  | `ts::Severity::Severe`
| `ts.Report.ERROR`   | `ts::Severity::Error`
| `ts.Report.WARNING` | `ts::Severity::Warning`
| `ts.Report.INFO`    | `ts::Severity::Info`
| `ts.Report.VERBOSE` | `ts::Severity::Verbose`
| `ts.Report.DEBUG`   | `ts::Severity::Debug`

The Python report classes support equivalent services as their C++ counterparts.
The log functions have only one form, using a string message only.
The C++ variable forms using formats are useless in Python, thanks to the `'%'`
string operator which performs the equivalent function.

The list of Python methods for the `ts.Report` subclasses is defined below.

| Python method | C++ equivalence
| ------------- | ---------------
| `setMaxSeverity(severity)` | `ts::Report::setMaxSeverity()`
| `log(severity, message)`   | `ts::Report::log()`
| `error(message)`           | `ts::Report::error()`
| `warning(message)`         | `ts::Report::warning()`
| `info(message)`            | `ts::Report::info()`
| `verbose(message)`         | `ts::Report::verbose()`
| `debug(message)`           | `ts::Report::debug()`
| `terminate()`              | `ts::AsyncReport::terminate()`

Example usage:
~~~
>>> rep = ts.StdErrReport()
>>> rep.info("info message")
* info message
>>> rep.verbose("verbose message")
>>> rep.setMaxSeverity(ts.Report.VERBOSE)
>>> rep.verbose("verbose message")
* verbose message
>>> 
~~~

## Transport stream processor  {#pytsp}

The Python class `ts.TSProcessor` is equivalent to the C++ class `ts::TSProcessor`.
It provides a way to start and stop a transport stream processing pipeline with the
same capabilities as the `tsp` command (which is only a thin wrapper on top of the
C++ class `ts::TSProcessor`).

The scenario of a transport stream processing session from Python is the following:

- Create an instance of some subclass of `ts.Report` to log messages.
  It is recommended to create an instance of `ts.AsyncReport` which is
  asynchronous and thread-safe.
- Create an instance of `ts.TSProcessor` using the report object as message logger.
- Set some attributes on this instance to define the processing options and plugins.
  See the list of attributes below.
- Start the processing using the method `start()`. This methods returns immediately.
  The transport stream processing pipeline runs in independent threads (one per plugin).
- If necessary, abort the processing using the method `abort()`. This method
  sends an abort request which is asynchronously processed. By default, the
  processing continues until its natural end of stream.
- Wait for the completion of the processing using the method `waitForTermination()`.

The attributes of the class `ts.TSProcessor` are listed in the following table.
Attributes must be set before calling `start()`.

| Attribute | Type | Default | Equivalent `tsp` option
| --------- | ---- | ------- | -----------------------
| `monitor`                  | `bool` | `False` | `--monitor`
| `ignore_joint_termination` | `bool` | `False` | `--ignore-joint-termination`
| `log_plugin_index`         | `bool` | `False` | `--log-plugin-index`
| `buffer_size`              | `int`  | 16 MB | `--buffer-size-mb` (the value of the attributes is in bytes)
| `max_flushed_packets`      | `int`  | `tsp` default | `--max-flushed-packets`
| `max_input_packets`        | `int`  | `tsp` default | `--max-input-packets`
| `initial_input_packets`    | `int`  | `tsp` default | `--initial-input-packets`
| `add_input_stuffing`       | `[int, int]` | `[0, 0]` | `--add-input-stuffing`
| `add_start_stuffing`       | `int`  | `0` | `--add-start-stuffing`
| `add_stop_stuffing`        | `int`  | `0` | `--add-stop-stuffing`
| `bitrate`                  | `int`  | `0` | `--bitrate` (computed by input plugin or PCR's by default)
| `bitrate_adjust_interval`  | `int`  | `5000` | `--bitrate-adjust-interval` (in milliseconds)
| `receive_timeout`          | `int`  | `0` | `--receive-timeout` (in milliseconds)
| `app_name`                 | `str`  | empty | application name, for help and error messages
| `input`                    | `[str]`   | `tsp` default | input plugin name and arguments (list of strings)
| `plugins`                  | `[[str]]` | none | packet processor plugins names and arguments (list of lists of strings)
| `output`                   | `[str]`   | `tsp` default | output plugin name and arguments (list of strings)

Example usage:
~~~
import ts
rep = ts.AsyncReport(severity = ts.Report.VERBOSE, timed_log = True)
tsp = ts.TSProcessor(rep)

tsp.add_input_stuffing = [1, 10]   # one null packet every 10 inputpackets
tsp.bitrate = 1000000              # nominal bitrate is 1 Mb/s
tsp.app_name = "demo"              # informational only, for log messages

# Set plugin chain.
tsp.input = ['craft', '--count', '1000', '--pid', '100', '--payload-pattern', '0123']
tsp.plugins = [
    ['until', '--packet', '100'],
    ['count'],
]
tsp.output = ['drop']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
rep.terminate()
~~~

Output for this example:
~~~
* 2020/10/17 18:50:08 - craft: initial input bitrate is 1,100,000 b/s
* 2020/10/17 18:50:08 - count: PID  100 (0x0064):         91 packets
* 2020/10/17 18:50:08 - count: PID 8191 (0x1FFF):          9 packets
~~~
Notes:
- One null packet is inserted every 10 input packets, adding 10% of artificial stuffing.
- Despite the input plugin generates 1000 packets, the `until` plugin terminates the processing after 100 packets.
- The user-specified bitrate is 1 Mb/s. But it applies to the input plugin. Since we add 10% stuffing,
  the global TS bitrate is 1.1 Mb/s as seen in the plugins.

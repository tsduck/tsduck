# Java and Python bindings  {#jpbindings}

# Overview  {#jpoverview}

Starting with version 3.25, TSDuck includes Java and Python bindings to some
high-level features.

Although subject to enhancements, these bindings will never aim at supporting the
full TSDuck feature set since this would be too large. Only a small subset of
TSDuck high-level features are targeted.

The Java classes are documented in the @ref java "Java bindings" reference section.

The Python classes are documented in the @ref python "Python bindings" reference section.

Sample [Java](https://github.com/tsduck/tsduck/tree/master/sample/sample-java)
and [Python](https://github.com/tsduck/tsduck/tree/master/sample/sample-python)
applications are available in the TSDuck source tree.

Currently, the TSDuck Java and Python bindings provide access to the following features.
Equivalences are provided between C++, Java, Python and command line tools.

The first three classes implement high-level features which have direct counterparts
as command line tools. The others are support classes which are only required to use
the high-level classes.

| Command line | C++ class                         | Java class                             | Python class
| ------------ | --------------------------------- | -------------------------------------- | ------------------------
| `tsp`        | `ts::TSProcessor`                 | `io.tsduck.TSProcessor`                | `tsduck.TSProcessor`
| `tsswitch`   | `ts::InputSwitcher`               | `io.tsduck.InputSwitcher`              | `tsduck.InputSwitcher`
| `tstabcomp`  | `ts::SectionFile`                 | `io.tsduck.SectionFile`                | `tsduck.SectionFile`
| n/a          | `ts::DuckContext`                 | `io.tsduck.DuckContext`                | `tsduck.DuckContext`
| n/a          | `ts::Report`                      | `io.tsduck.AbstractSyncReport`         | `tsduck.AbstractSyncReport`
| n/a          | `ts::AsyncReport`                 | `io.tsduck.AbstractAsyncReport`        | `tsduck.AbstractAsyncReport`
| n/a          | `ts::SystemMonitor`               | `io.tsduck.SystemMonitor`              | `tsduck.SystemMonitor`
| n/a          | `ts::PluginEventHandlerInterface` | `io.tsduck.AbstractPluginEventHandler` | `tsduck.AbstractPluginEventHandler`
| n/a          | `ts::PluginEventContext`          | `io.tsduck.PluginEventContext`         | `tsduck.PluginEventContext`

# Support classes  {#jpsupportclasses}

## TSDuck execution context  {#jpduckctx}

The `DuckContext` class is used to define and accumulate regional or operator preferences.
In the TSDuck C++ programming guide, it is referred to as _TSDuck execution context_.
Most of the time, using the default state of a new instance is sufficient.

The application _sample Japanese tables_, available in
[Java](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleJapaneseTables.java)
and [Python](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-japanese-tables.py),
demonstrates how it can be necessary to override the defaults in specific cases.

## Reporting classes  {#jpreporting}

The reporting classes (`ts::Report` C++ class hierarchy) are used to report logs,
errors and debug. They are consistently used all over TSDuck and are required to
use the high level features. There is a large hierarchy of classes in the three
languages which can be classified according to two sets of criteria:

- Synchronous vs. asynchronous:
  - Synchronous report classes log messages in the same thread as the caller.
    They are usually not thread-safe.
  - Asynchronous report classes, on the other hand, can be used in a multi-threaded
    environment and the actual message logging (such as writing in a log file) is
    performed in a separate thread. As a consequence, an asynchronous report
    instance must be explicitly _terminated_. An asynchronous report class is
    required when using heavily multi-threaded classes such as `TSProcessor`
    or `InputSwitcher`.

- Native vs. abstract:
  - Native classes are the C++ classes which are used in all the TSDuck command line
    tools. They are typically used to report to standard output, standard error,
    files or dropping the logs. They can be used from Java and Python directly but
    cannot be derived or customized. They are typically used when predefined error
    logging is sufficient.
  - Abstract classes are pure Java or Python base classes which are designed to be
    derived in applications. Such application-defined classes shall override the
    method `logMessageHandler` (Java) or `log` (Python) to intercept and process the
    message lines.

The asynchronous abstract classes can be useful to collect events, tables and
sections in XML, JSON or binary / hexadecimal form in Java or Python applications
when using `TSProcessor` or `InputSwitcher`. Some of the sample Java and Python
applications illustrate this mechanism.

| Category               | C++ class         | Java class                      | Python class
| ---------------------- | ----------------- | ------------------------------- | ------------------------
| Synchronous, native    | `ts::CerrReport`  | `io.tsduck.ErrReport`           | `tsduck.StdErrReport`
|                        | `ts::NullReport`  | `io.tsduck.NullReport`          | `tsduck.NullReport`
| Asynchronous, native   | `ts::AsyncReport` | `io.tsduck.AsyncReport`         | `tsduck.AsyncReport`
| Synchronous, abstract  | `ts::Report`      | `io.tsduck.AbstractSyncReport`  | `tsduck.AbstractSyncReport`
| Asynchronous, abstract | `ts::AsyncReport` | `io.tsduck.AbstractAsyncReport` | `tsduck.AbstractAsyncReport`

## Resource monitoring  {#jpmonitor}

The `SystemMonitor` class is available in all languages, C++, Java and Python.
It can be used at the top-level of an application to implement the `--monitor`
option as found in `tsp` and `tsswitch`. An instance of a thread-safe `Report`
class is used to report monitoring messages.

The `SystemMonitor` class is very simple to used. Examples are available in
[Java](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleMonitoring.java) and
[Python](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-monitoring.py).

## Plugin events  {#jpevents}

For developers, TSDuck plugins can _signal events_ which can be handled by the application.
Each event is signalled with a user-defined 32-bit _event code_.
An application can register _event handlers_ in the `ts::TSProcessor` instance
(see the class `ts::PluginEventHandlerRegistry`, knowing that `ts::TSProcessor`
is a subclass of `ts::PluginEventHandlerRegistry`).
The event handler registration can include various _selection criteria_ such as
event code value or originating plugin (see the inner class
`ts::PluginEventHandlerRegistry::Criteria`).

C++ developers who create their own plugins can signal any kind of event that they
later handle in their application. This is illustrated in the
[C++ sample custom application](https://github.com/tsduck/tsduck/blob/master/sample/sample-app-custom/myexec.cpp).
In this sample code, everything is customized in the application: the plugin,
the event it signals, the associated event data, the application handling of the event.

Since developing a TSDuck plugin is only possible in C++, Java and Python developers have
more limited options. Some standard TSDuck plugins such as `tables`, `psi` or `mpe` provide
the option `--event-code`. Using this option, the plugins signal event using the specified
event code for each data they handle (sections or MPE datagrams depending on the plugin).

Java and Python applications can derive from class `AbstractPluginEventHandler` to define
and register their own event handlers. Thus, binary sections or MPE datagrams can be
handled directly from the plugin to the Java or Python application.

Some plugins are even dedicated to application developers and are useless on `tsp` command lines.
This is the case of the `memory` plugin (both an input and an output plugin).
This plugin, when used in a `TSProcessor` instance, performs direct transport stream input
and output from and to the application using memory buffers. The memory buffers are
signalled using plugin events. The `memory` input plugin is an example of an
application-defined event handler returning data to the plugin.
See [sample code](https://github.com/tsduck/tsduck/blob/master/sample/sample-memory-plugins/)
in the TSDuck source code tree.

# Communication between Java or Python applications and their plugins  {#jpplugincomm}

At high level, Java and Python applications can only run `TSProcessor` or `InputSwitcher`
sessions, just like a shell-script would do with commands `tsp` and `tsswitch`.

The communication from the Java and Python applications to the plugins is performed using
plugin options. These options may contain file names or UDP ports which can be created
by the application.

More effectively, most file contents can be provided directly on the command line, avoiding
the burden of creating temporary files. For instance, wherever an input XML file name is
expected, it is possible to use the XML content instead. Any "XML file name" which starts
with "<?xml", it is considered as inline XML content. Similarly, if an input JSON file name
starts with "{" or "[", it is considered as inline JSON content.

On reverse side, there is some limited form of communication from the plugins to the Java or
Python application. There are basically two ways to handle plugin information in the application:
the logging system and plugin events.

Using the logging system:
Some plugins support options such as `--log-xml-line`, `--log-json-line` or `--log-hexa-line`.
With these options, the extracted data (table, section, MPE datagram) are "displayed" as one
single line in the designated format on the logging system. Using user-defined Java or Python
@ref jpreporting "asynchronous abstract reporting classes", the application receives all logged
lines and can filter and manipulate the data which were extracted and logged by the plugins.

Using plugin events:
Some plugins support the option `--event-code`. With this option, the extracted data are
_signalled_ by the plugin as an event. Using and registering user-defined Java or Python
@ref jpevents "plugin event handlers", the application is directly notified of the data.

Which mechanism, logging system or plugin events, should be used depends on the application.

- Logging system:
  - Pros:
    - The log lines are asynchronously processed in the context of the low-priority logging
      thread. Any lengthy processing in the Java or Python application does not hurt the
      dynamics of the plugins.
  - Cons:
    - If the application needs to process binary data, the additional serialization process
      in the log line adds some useless overhead.
    - Because the logging system is non-intrusive by design, log messages may be lost if there
      are more messages than the logging thread can process without making plugin threads wait.
      This can be mitigated using the _synchronous log_ option in the `AbstractAsyncReport`
      consttructor.
- Plugin events:
  - Pros:
    - The binary data are directly passed from the plugin to the application without any
      serialization, logging or multi-threading overhead.
  - Cons:
    - The application-defined event handlers execute in the context of the plugin thread.
      Any lengthy processing at this stage slows down the plugin.

The following sample applications can be used as a starting point:

| Communication type | Java | Python
| ------------------ | ---- | ------
| Logging (XML) | [SampleAnalyzeSDT](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleAnalyzeSDT.java) | [sample-analyze-sdt.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-analyze-sdt.py)
| Logging (JSON) | [SampleAnalyzeTS](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleAnalyzeTS.java) | [sample-analyze-ts.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-analyze-ts.py)
| Logging (bin/hexa) | [SampleFilterTablesLog](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleFilterTablesLog.java) | [sample-filter-tables-log.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-filter-tables-log.py)
| Plugin events (sections) | [SampleFilterTablesEvent](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleFilterTablesEvent.java) | [sample-filter-tables-event.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-filter-tables-event.py)
| Plugin events (MPE datagrams) | [SampleMPE](https://github.com/tsduck/tsduck/blob/master/sample/sample-java/SampleMPE.java) | [sample-mpe.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-python/sample-mpe.py)
| Plugin events (input/output) | [SampleMemoryPlugins](https://github.com/tsduck/tsduck/blob/master/sample/sample-memory-plugins/SampleMemoryPlugins.java) | [sample-memory-plugins.py](https://github.com/tsduck/tsduck/blob/master/sample/sample-memory-plugins/sample-memory-plugins.py)

# Using TSDuck Java bindings  {#javausing}

All TSDuck Java classes are defined in a package named `io.tsduck`.

A few examples are provided in the directory
[`sample/sample-java`](https://github.com/tsduck/tsduck/tree/master/sample/sample-java)
in the TSDuck source code package.

## Linux  {#javalinux}

The TSDuck Java bindings are installed with TSDuck in `/usr/share/tsduck/java`.
All classes are in a JAR file named `tsduck.jar`. Simply add this JAR in the
environment variable `CLASSPATH` to use TSDuck from any Java application:

~~~
export CLASSPATH="/usr/share/tsduck/java/tsduck.jar:$CLASSPATH"
~~~

## macOS  {#javamac}

This is similar to Linux, except that `/usr/local/share` is used instead of `/usr/share`.

~~~
export CLASSPATH="/usr/local/share/tsduck/java/tsduck.jar:$CLASSPATH"
~~~

## Windows  {#javawin}

On Windows, Java bindings are optional components of the TSDuck installer.
When they are selected for installation, they are installed in the TSDuck
area and the environment variable `CLASSPATH` is modified at system level
to include the JAR file of the TSDuck Java bindings. Thus, any Java program
can use TSDuck directly.

# Using TSDuck Python bindings  {#pyusing}

All TSDuck bindings are defined in a module named `tsduck`.
All Python programs using TSDuck shall consequently start with:
~~~
import tsduck
~~~

A few examples are provided in the directory
[`sample/sample-python`](https://github.com/tsduck/tsduck/tree/master/sample/sample-python)
in the TSDuck source code package.

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
these C++ functions are hidden inside the Python bindings and invisible to the
C++ application developer.

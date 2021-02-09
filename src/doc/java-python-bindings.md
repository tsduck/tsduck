# Java and Python bindings  {#jpbindings}
[TOC]

# Overview  {#jpoverview}

Starting with version 3.25, TSDuck includes Java and Python bindings to some
high-level features.

Although subject to evolution, these bindings will never aim at supporting the
full TSDuck feature set since this would be too large. Only a small subset of
TSDuck high-level features are targeted.

The Java classes are documented in the @ref java "Java bindings" reference section.

The Python classes are documented in the @ref python "Python bindings" reference section.

Sample [Java](https://github.com/tsduck/tsduck/tree/master/sample/sample-java)
and [Python](https://github.com/tsduck/tsduck/tree/master/sample/sample-python)
applications are available in the TSDuck source tree.

Currently, the TSDuck Java and Python bindings provide access to the following features.
Equivalences are provided between C++, Java, Python and command line tools.

| Command line | C++ class           | Java class                      | Python class
| ------------ | ------------------- | ------------------------------- | ------------------------
| `tsp`        | `ts::TSProcessor`   | `io.tsduck.TSProcessor`         | `ts.TSProcessor`
| `tsswitch`   | `ts::InputSwitcher` | `io.tsduck.InputSwitcher`       | `ts.InputSwitcher`
| `tstabcomp`  | `ts::SectionFile`   | `io.tsduck.SectionFile`         | `ts.SectionFile`
| n/a          | `ts::DuckContext`   | `io.tsduck.DuckContext`         | `ts.DuckContext`
| n/a          | `ts::Report`        | `io.tsduck.AbstractSyncReport`  | `ts.AbstractSyncReport`
| n/a          | `ts::AsyncReport`   | `io.tsduck.AbstractAsyncReport` | `ts.AbstractAsyncReport`

The first set of classes implements high-level features which have direct counterparts
as command line tools. The last set contains support classes which are only required
to use the high-level classes.

# Support classes

The `DuckContext` class is used to define and accumulate regional preferences.
Most of the time, using the default state of a new instance is sufficient.
The application "sample Japanese tables", available in Java and Python,
demonstrates how it can be necessary to override the defaults in specific cases.

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
    instance must be explicitly "terminated". An asynchronous report class is
    required when using heavily multi-threaded classes such as `TSProcessor`
    or `InputSwitcher`.
 
- Native vs. abstract:
  - Native classes are the C++ classes which are used in all the TSDuck command line
    tools. They are typically used to report to standard output, standard error,
    files or dropping the logs. They can be used from 
  - Abstract classes are pure Java or Python base classes which are designed to be
    derived in the applications. Such application-defined classes shall override
    the method `logMessageHandler` (Java) or `log` (Python) to intercept and process
    the message lines.

The asynchronous abstract classes can be useful to collect events, tables and
sections in XML, JSON or binary / hexadecimal form in Java or Python applications
when using `TSProcessor` or `InputSwitcher`.

| Category               | C++ class         | Java class                      | Python class
| ---------------------- | ----------------- | ------------------------------- | ------------------------
| Synchronous, native    | `ts::CerrReport`  | `io.tsduck.ErrReport`           | `ts.StdErrReport`
|                        | `ts::NullReport`  | `io.tsduck.NullReport`          | `ts.NullReport`
| Asynchronous, native   | `ts::AsyncReport` | `io.tsduck.AsyncReport`         | `ts.AsyncReport`
| Synchronous, abstract  | `ts::Report`      | `io.tsduck.AbstractSyncReport`  | `ts.AbstractSyncReport`
| Asynchronous, abstract | `ts::AsyncReport` | `io.tsduck.AbstractAsyncReport` | `ts.AbstractAsyncReport`

# Using TSDuck Java bindings  {#javausing}

All TSDuck Java classes are defined in a package named `io.tsduck`.

A few examples are provided in the directory `sample/sample-java` in the TSDuck
source code package.

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

All TSDuck bindings are defined in a module named `ts`.
All Python programs using TSDuck shall consequently start with:
~~~
import ts
~~~

A few examples are provided in the directory `sample/sample-python` in the TSDuck
source code package.

Warning: Do to the structure of Python modules and how they are managed by Doxygen,
the Python classes are documented with intermediate names such as `ts.tsp.TSProcessor`
but the actual name to use in applications should be `ts.TSProcessor`.

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

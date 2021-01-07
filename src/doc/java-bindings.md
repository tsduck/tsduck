# Java bindings  {#javabindings}
[TOC]

# Overview  {#javaoverview}

Starting with version 3.25, TSDuck includes Java bindings to some
high-level features.

Although subject to evolution, these bindings will never aim at
supporting the full TSDuck feature set since this would be too large.
Only a small subset of TSDuck high-level features are targeted.

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

# TSDuck Java bindings reference  {#javaref}

All TSDuck Java classes are defined in a package named `io.tsduck`.

The Java classes are documented in the @ref java "Java bindings" reference section.

# Sample application using TSDuck memory plugins: Makefile for UNIX systems.
# The TSDuck development package must have been installed.

CXXFLAGS    += $(shell tsconfig --cflags)
LDLIBS      += $(shell tsconfig --libs)
JAVAC       ?= javac
JAVAC_FLAGS ?=

export CLASSPATH  = $(shell tsconfig --java):../sample-java:
export PYTHONPATH = $(shell tsconfig --python)

default: sample-memory-plugins SampleMemoryPlugins.class

%.class: %.java
	$(JAVAC) $(JAVAC_FLAGS) $<

test: test-cpp test-java test-python

test-cpp: sample-memory-plugins
	@echo "==== C++ version"
	./sample-memory-plugins

test-java: SampleMemoryPlugins.class
	@echo "==== Java version"
	java SampleMemoryPlugins

test-python:
	@echo "==== Python version"
	./sample-memory-plugins.py

clean:
	@rm -rf *.o *.class *.ts

distclean: clean
	@rm -rf sample-memory-plugins

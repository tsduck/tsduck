# Sample application using TSDuck as a library: Makefile for UNIX systems.
#
# The TSDuck development package must have been installed.
#
# By default, the sample application is built against the TSDuck dynamic
# library in /usr/bin. Define TS_STATIC to link against the TSDuck static
# library: make TS_STATIC=true

include /usr/include/tsduck/tsduck.mk

default: myexec

clean:
	rm -rf *.o
distclean: clean
	rm -rf myexec

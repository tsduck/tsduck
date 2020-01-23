# Sample application using TSDuck as a library: Makefile for UNIX systems.
#
# The TSDuck development package must have been installed.
#
# By default, the sample application is built against the TSDuck dynamic
# library in /usr/bin. Define TS_STATIC to link against the TSDuck static
# library: make TS_STATIC=true

ifeq ($(shell uname -s),Darwin) # Mac
    include /usr/local/include/tsduck/tsduck.mk
else # Linux
    include /usr/include/tsduck/tsduck.mk
endif

default: myexec

clean:
	rm -rf *.o *.ts
distclean: clean
	rm -rf myexec

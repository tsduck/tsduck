# Sample application using TSDuck as a library: Makefile for UNIX systems.
# The TSDuck development package must have been installed.

CXXFLAGS += $(shell tsconfig --cflags)
LDLIBS += $(shell tsconfig --libs)

default: myexec

clean:
	@rm -rf *.o *.ts
distclean: clean
	@rm -rf myexec

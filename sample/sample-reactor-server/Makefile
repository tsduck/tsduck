# Sample application using TSCore as a library: Makefile for UNIX systems.
# The TSDuck development package must have been installed.

CXXFLAGS += $(shell tsconfig --tscore --cflags)
LDLIBS += $(shell tsconfig --tscore --libs)

default: reactor-server

clean distclean:
	@rm -rf *.o *.ts reactor-server

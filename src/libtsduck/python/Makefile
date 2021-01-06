# Dummy makefile for subdirectory in vpath. Actual make is under control of parent directory.

DONT_BUILD_DEPS := TRUE
include ../../../Makefile.tsduck

NORECURSE_SUBDIRS += ts
EXTRA_DISTCLEAN += ts/__pycache__ ts/*/__pycache__ ts/*/*/__pycache__

default:
	@true

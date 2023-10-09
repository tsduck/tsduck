#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
#
#  Top-level qmake project file for TSDuck.
#
#-----------------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered
TSDIRS = $$system(find . -type d -name ts\\* -prune)
SUBDIRS += libtsduck $$sorted(TSDIRS) utest

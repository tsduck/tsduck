#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Top-level qmake project file for TSDuck.
#
#-----------------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered
TSDIRS = $$system(cd $$_PRO_FILE_PWD_; find . -maxdepth 1 -type d -name ts\\*)
TSDIRS = $$replace(TSDIRS,./,)
TSDIRS = $$replace(TSDIRS,tsxml,)
TSDIRS = $$sorted(TSDIRS)
SUBDIRS += libtsduck tsxml $$TSDIRS utest

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

equals($$system($$_PRO_FILE_PWD_/../dtapi-config.sh --support), '') {
    # No Dektec support
    LIBSDIR = libtscore libtsduck
    TSDIRS = $$system(cd $$_PRO_FILE_PWD_ ; find . -maxdepth 1 -type d -name ts\\* ! -name \\*dektec\\*)
}
else {
    # Dektec support
    LIBSDIR = libtscore libtsduck libtsdektec
    TSDIRS = $$system(cd $$_PRO_FILE_PWD_ ; find . -maxdepth 1 -type d -name ts\\*)
}

TSDIRS = $$replace(TSDIRS,./,)
TSDIRS = $$sorted(TSDIRS)
SUBDIRS += $$LIBSDIR $$TSDIRS utest

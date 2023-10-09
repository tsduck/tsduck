#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
#
#  Alias to the TSDuck Python bindings.
#
#  Originally, the TSDuck Python bindings were named "ts". This created an
#  issue with doxygen which is unable to correctly process classes with the
#  same full name in different languages, e.g. ts::MyClass (C++) and
#  ts.MyClass (Python).
#
#  The TSDuck Python bindings were renamed in module "tsduck" to avoid this
#  documentation issue. To preserve compatibility with older Python code,
#  the module "ts.py" imports all symbols from "tsduck.py". Thus, TSDuck
#  Python features can be indifferently used from "tsduck" or "ts" while
#  the official module name is now "tsduck".
#
#-----------------------------------------------------------------------------

from tsduck import *

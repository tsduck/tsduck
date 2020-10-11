#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------


__all__ = []
__author__ = 'Thierry Lelegard'
__version__ = '(see later)'

import os
import ctypes
import ctypes.util


# Search the TSDuck library.
def search_libtsduck():
    if os.name == 'nt':
        base = 'tsduck.dll'
        search = [os.getenv('TSDUCK','')]
        search.extend(os.getenv('Path','').split(os.pathsep))
    else:
        base = 'libtsduck.so'
        # For macOS only: LD_LIBRARY_PATH is not passed to shell-scripts for security reasons.
        # A backup version is defined in build/setenv.sh to test development versions.
        search = os.getenv('LD_LIBRARY_PATH2','').split(os.pathsep)
        search.extend(os.getenv('LD_LIBRARY_PATH','').split(os.pathsep))

    # Search the TSDuck library.
    path = ''
    for dir in (dir for dir in search if dir != ''):
        file = dir + os.sep + base
        if os.path.exists(file):
            return file

    # If not found in various explicit paths, try system search.
    return ctypes.util.find_library(base)


# Load the TSDuck library.
_lib = ctypes.CDLL(search_libtsduck())


# An internal utility class to encapsulate a binary buffer returning data or string from a C++ function.
# The C++ function typically use (uint8_t* buffer, size_t* size) parameters. The size parameter contains
# the maximum buffer size on input and the actual returned data size on output.
class OutByteBuffer:
    # Constructor with maximum buffer size in bytes.
    def __init__(self, size):
        self._size = ctypes.c_size_t(size)
        self._data = bytearray(self._size.value)

    # "uint8_t* buffer" parameter for the C++ function.
    def data_ptr(self):
        carray_type = ctypes.c_uint8 * self._size.value
        return ctypes.cast(carray_type.from_buffer(self._data), ctypes.POINTER(ctypes.c_uint8))

    # "size_t* size" parameter for the C++ function.
    def size_ptr(self):
        return ctypes.byref(self._size)

    # Get the returned data as a byte array.
    def to_bytearray(self):
        return self._data[:self._size.value]

    # Get the returned data as a UTF-16 string.
    def to_string(self):
        return self._data[:self._size.value].decode("utf-16")


# Load names from "ts" submodules as part of ts namespace.
from .info import *

# Now update module version.
__version__ = version()

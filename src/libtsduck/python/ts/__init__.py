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


import os
import ctypes
import ctypes.util

# Name of the TSDuck library.
if os.name == 'nt':
    _lib_base = 'tsduck.dll'
    _lib_search = [os.getenv('TSDUCK','')]
    _lib_search.extend(os.getenv('Path','').split(os.pathsep))
else:
    _lib_base = 'libtsduck.so'
    # For macOS only: LD_LIBRARY_PATH is not passed to shell-scripts for security reasons.
    # A backup version is defined in build/setenv.sh to test development versions.
    _lib_search = os.getenv('LD_LIBRARY_PATH2','').split(os.pathsep)
    _lib_search.extend(os.getenv('LD_LIBRARY_PATH','').split(os.pathsep))

# Search the TSDuck library.
_lib_path = ''
for __dir in _lib_search:
    if __dir != '':
        __file = __dir + os.sep + _lib_base
        if os.path.exists(__file):
            _lib_path = __file
            break

# If not found in various explicit paths, try system search.
if _lib_path == '':
    _lib_path = ctypes.util.find_library(_lib_base)

# Load the TSDuck library.
_lib = ctypes.CDLL(_lib_path)

# Load names from "ts" submodules as part of ts namespace.
from .info import *

# Prevent application namespace pollution: nothing can be exported by "from ts import *".
__all__ = []

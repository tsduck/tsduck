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

import ts
import ctypes

#
# C function: uint32_t tspyVersionInteger()
# Python function (direct binding): intVersion()
#
intVersion = ts._lib.tspyVersionInteger
intVersion.restype = ctypes.c_uint32
intVersion.argtypes = []

#
# C function: void tspyVersionString(uint8_t* buffer, size_t* size)
# Python function: version()
#
tspyVersionString = ts._lib.tspyVersionString
tspyVersionString.restype = None
tspyVersionString.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_size_t)]

def version():
    size = ctypes.c_size_t(64)
    buf = bytearray(size.value)
    buf_ptr = ctypes.cast(ctypes.create_string_buffer(bytes(buf)), ctypes.POINTER(ctypes.c_uint8)).contents
    tspyVersionString(buf_ptr, ctypes.byref(size))
    print("size:", size.value, "buf:", buf[0], buf[1], buf[2], buf[3], buf[4])
    return buf[:size.value].decode("utf-16")

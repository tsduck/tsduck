#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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
#
#  TSDuck Python bindings: base class to interface C++ objects.
#
#-----------------------------------------------------------------------------

import ctypes

##
# This interface is implemented by all TSDuck classes which are backed by a C++ object.
# @ingroup python
#
# There is an inherent problem when garbage-collected languages such as Python are interfaced
# with languages using explicit memory management. When a Python class encapsulates a native
# C++ object of the corresponding C++ class, when should the C++ object be deleted?
# This is a problem which has been discussed many times on the Internet and the answer
# is disappointing: there is no good solution.
# 
#  1. A naive approach would be to override __del__(self) and perform the C++ deallocation here.
#     It is well known that __del__() creates more issues than it solves. Specifically, we cannot
#     guarantee the order of finalization of objects, which could lead to crashes when C++ objects
#     reference each other is a predetermined order. 
#
#  2. Never delete C++ objects and let them accumulate. This can be acceptable if a
#     guaranteed maximum number of C++ objects are allocated during the life of the
#     application and the corresponding memory usage is acceptable.
# 
#  3. Expose a public method in all Python classes which deletes, frees, deallocates, you name it,
#     the encapsulated C++ object. It is then the responsibility of the application to call
#     this method on time. This is counter-intuitive to both Python and C++ programmers but
#     this is the price to pay when you want to use them together.
#
# In the Python TSDuck bindings, all classes which encapsulate a C++ object implement the
# interface NativeObject which provides the delete() method to explicitly delete the C++
# object. In practice, users have the choice between solutions 2 or 3.  
#
class NativeObject:

    ##
    # Constructor for subclasses.
    #
    def __init__(self):
        # Storage of a pointer to the C++ object.
        # Subclasses constructors should initialize it using the result of some native function.
        self._native_object = ctypes.c_void_p(0)

    ##
    # Explicitly free the underlying C++ object.
    # After this call, the object becomes unusable.
    # Most usages are unpredictable but most likely will do nothing.
    # @return None.
    #
    def delete(self):
        # Subclasses constructors should free it using some native function
        # and then call super().delete() to cleanup the field.
        self._native_object = ctypes.c_void_p(0)

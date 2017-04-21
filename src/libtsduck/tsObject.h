//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for objects which can be stored in a repository
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSafePtr.h"

namespace ts {

    // Safe pointer for Object
    class Object;
    typedef SafePtr <Object, Mutex> ObjectPtr;

    class TSDUCKDLL Object
    {
    public:

        // Virtual destructor
        virtual ~Object() {}

        // Store a safe pointer to an Object in a static thread-safe repository.
        // Each stored pointer is associated to a name.
        // Return the previous value which was associated to that name or a null
        // pointer when not previously assigned.
        static ObjectPtr StoreInRepository (const std::string& name, const ObjectPtr& value);

        // Get the safe pointer to an Object in the static thread-safe repository
        // which is associated to the specified name.
        // Return a null pointer when not found, ie. ObjectPtr::isNull() is true.
        static ObjectPtr RetrieveFromRepository (const std::string& name);

        // Erase from the static thread-safe repository the value which is associated to
        // the specified name.
        static void EraseFromRepository (const std::string& name);

    private:
        // Static thread-safe repository of Object
        static std::map <std::string, ObjectPtr> _repository;

        // Mutex protecting access to the static thread-safe repository of Object
        static Mutex _repository_mutex;
    };
}

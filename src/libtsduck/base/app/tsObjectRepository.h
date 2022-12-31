//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  A global repository of general-purpose base class for polymophic objects.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingletonManager.h"
#include "tsObject.h"

namespace ts {
    //!
    //! A global repository of general-purpose base class for polymophic objects.
    //! @ingroup cpp
    //!
    //! The repository is a thread-safe singleton. It can be used as a central
    //! repository of user-defined objects which is shared by all modules, all
    //! plugins, all threads.
    //!
    class TSDUCKDLL ObjectRepository
    {
        TS_DECLARE_SINGLETON(ObjectRepository);
    public:
        //!
        //! Store a safe pointer to an Object (or typically a subclass thereof) in the repository.
        //! @param [in] name Each stored pointer is associated to a name.
        //! @param [in] value Safe-pointer to the object to store.
        //! @return The previous value which was associated to that name or a null
        //! pointer when not previously assigned.
        //!
        ObjectPtr store(const UString& name, const ObjectPtr& value);

        //!
        //! Get the safe pointer to an Object in the repository.
        //! @param [in] name Name which is associated to the object.
        //! @return A safe-pointer to the stored object or a null pointer when not found.
        //!
        ObjectPtr retrieve(const UString& name);

        //!
        //! Erase an object from the repository.
        //! @param [in] name Name which is associated to the object to erase.
        //!
        void erase(const UString& name);

    private:
        Mutex _mutex;
        std::map<UString, ObjectPtr> _repository;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A global repository of general-purpose base class for polymophic objects.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"
#include "tsObject.h"
#include "tsUString.h"

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
        std::mutex _mutex {};
        std::map<UString, ObjectPtr> _repository {};
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose base class for objects with an "owner".
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsFatal.h"

namespace ts {
    //!
    //! General-purpose base class for objects with an "owner".
    //! @ingroup libtscore cpp
    //!
    //! An object A is "owned" when we associate this object with an "owner" object B
    //! during the contruction of A. This association cannot be modified. Typically,
    //! A is a member of B and B always has a (slightly) longer lifetime than A.
    //!
    class TSCOREDLL OwnedObject: public Object
    {
    public:
        //!
        //! Constructor.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        OwnedObject(Object* owner = nullptr) : _owner(owner) {}

        //!
        //! Destructor.
        //!
        virtual ~OwnedObject() override;

        // Copy / move constructors and assignments must be carefully addressed.
        // The owner object is a invariant property of an object, it is not part of the value of the object.
        // Therefore, the address of owner object shall never be copied. Copy/move constructors are deleted
        // because we need an owner object on construction. Copy/move assignments do nothing because the
        // OnwedObject itself holds no value, only it invariant property of owned object.
        //! @cond nodoxygen
        OwnedObject(OwnedObject&&) = delete;
        OwnedObject(const OwnedObject&) = delete;
        OwnedObject& operator=(OwnedObject&&) { return *this; }
        OwnedObject& operator=(const OwnedObject&) { return *this; }
        //! @endcond

        //!
        //! Get the address of the optional "owner" object which was specified in the constructor.
        //! @return Address of the "owner" object or a null pointer if there was none.
        //!
        Object* owner() { return _owner; }

        //!
        //! Check if the object is owned.
        //! @return True if this object has an owner, false otherwise.
        //!
        bool isOwned() { return _owner != nullptr; }

        //!
        //! Get the address of the "owner" object which was specified in the constructor.
        //! This template version requires that the owner objet is set and of type OBJECT, or some subclass of it.
        //! If there is no owner object or if it is not compatible with the template class OBJECT, this is a fatal
        //! error and the application is terminated.
        //! @tparam OBJECT A subclass of Object
        //! @return Address of the "owner" object or a null pointer if there was none.
        //!
        template <class OBJECT> requires std::derived_from<OBJECT, ts::Object>
        OBJECT* owner();

        //!
        //! Check if the object is owned by an object of a given type.
        //! @tparam OBJECT A subclass of Object
        //! @return True if this object has an owner by an object of type OBJECT, false otherwise.
        //!
        template <class OBJECT> requires std::derived_from<OBJECT, ts::Object>
        bool isOwned() { return dynamic_cast<OBJECT*>(_owner) != nullptr; }

    private:
        Object* _owner = nullptr;
    };
}


//----------------------------------------------------------------------------
// Definition of template methods.
//----------------------------------------------------------------------------

template <class OBJECT> requires std::derived_from<OBJECT, ts::Object>
OBJECT* ts::OwnedObject::owner()
{
    OBJECT* obj = dynamic_cast<OBJECT*>(_owner);
    if (obj == nullptr) {
        TS_FATAL("internal error, incorrect owner object type in reactive class");
    }
    return obj;
}

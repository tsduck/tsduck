//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Managed pointers for COM objects, auto-released (Windows-specific)
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsComUtils.h"
#include "tsCerrReport.h"
#include "tsFormat.h"

namespace ts {
    //!
    //! Managed pointers for COM objects, auto-released (Windows-specific).
    //! @tparam COMCLASS A COM interface or object class.
    //!
    template <class COMCLASS>
    class ComPtr
    {
    private:
        // Encapsulated pointer to COM object
        COMCLASS* _ptr;

    public:
        //!
        //! Default constructor.
        //! @param [in] p Address of a COM object. If @a p is not null and @a hr is success,
        //! the COM object becomes managed by ComPtr. Its reference count is unchanged.
        //! @param [in] hr An @c HRESULT value, typically the returned status of the system
        //! call which created @a p.
        //!
        ComPtr(COMCLASS* p = 0, ::HRESULT hr = S_OK) :
            _ptr(SUCCEEDED(hr) ? p : 0)
        {
        }

        //!
        //! Copy constructor.
        //! The reference count of the COM object is incremented.
        //! @param [in] p Another ComPtr instance.
        //!
        ComPtr(const ComPtr<COMCLASS>& p) :
            _ptr(p.pointer())
        {
            if (_ptr != 0) {
                _ptr->AddRef();
            }
        }

        //!
        //! Constructor using CoCreateInstance().
        //! If the COM object is successfully created, it becomes managed and
        //! its reference count is unchanged (== 1).
        //!
        //! @param [in] class_id Class id of the COM object to create.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in] report Where to report errors.
        //!
        //! Example:
        //! @code
        //! ComPtr<::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
        //! @endcode
        //!
        ComPtr(const ::IID& class_id, const ::IID& interface_id, ReportInterface& report = CERR) :
            _ptr(0)
        {
            createInstance(class_id, interface_id, report);
        }

        //!
        //! Destructor.
        //! The COM object is released (its reference count is decremented).
        //!
        ~ComPtr()
        {
            release();
        }

        //!
        //! Check if null pointer.
        //! @return True if this is a null pointer (no object).
        //!
        bool isNull() const
        {
            return _ptr == 0;
        }

        //!
        //! Dereference operator.
        //! To access a COM object.
        //! @return A reference to the COM object.
        //!
        COMCLASS& operator*() const
        {
            return *_ptr;
        }

        //!
        //! To access a COM object pointer, without releasing it.
        //! @return A pointer to the COM object.
        //!
        COMCLASS* pointer() const
        {
            return _ptr;
        }

        //!
        //! Dereference operator.
        //! To access members of COM objects.
        //! @return A pointer to the COM object.
        //!
        COMCLASS* operator->() const
        {
            return _ptr;
        }

        //!
        //! Release previous pointer, return a receiver for new pointer.
        //! Typically used in CoCreateInstance() and COM methods returning a new COM interface.
        //! @return The address of the internal COM object pointer.
        //!
        COMCLASS** creator()
        {
            release();
            return &_ptr;
        }

        //!
        //! Release the COM object, its reference count is decremented.
        //! The pointer becomes null.
        //!
        void release()
        {
            if (_ptr != 0) {
                _ptr->Release();
                _ptr = 0;
            }
        }

        //!
        //! Assignment from a ComPtr to a subclass.
        //! The reference count of the COM object is incremented.
        //! @tparam COMSUBCLASS A COM interface or object class, a subclass of @a COMCLASS.
        //! @param [in] p A ComPtr to a @a COMSUBCLASS object.
        //! @return A reference to this object.
        //!
        template <class COMSUBCLASS>
        ComPtr<COMCLASS>& assign(const ComPtr<COMSUBCLASS>& p)
        {
            release();
            _ptr = p.pointer();
            if (_ptr != 0) {
                _ptr->AddRef();
            }
            return *this;
        }

        //!
        //! Assignment operator.
        //! The reference count of the COM object is incremented.
        //! @param [in] p A ComPtr to a COM object.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& operator=(const ComPtr<COMCLASS>& p)
        {
            if (&p != this) {
                assign(p);
            }
            return *this;
        }

        //!
        //! Assignment operator from a COM object pointer.
        //! The COM object becomes managed. Its reference count is unchanged.
        //! @param [in] p Address of a COM object. Can be null.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& operator=(COMCLASS* p)
        {
            release();
            _ptr = p;
            return *this;
        }

        //!
        //! Assign using CoCreateInstance().
        //! If the COM object is successfully created, it becomes managed and
        //! its reference count is unchanged (== 1).
        //!
        //! @param [in] class_id Class id of the COM object to create.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in] report Where to report errors.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& createInstance(const ::IID& class_id, const ::IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            ::HRESULT hr = ::CoCreateInstance(class_id,               // Class ID for object
                                              NULL,                   // Not part of an aggregate
                                              ::CLSCTX_INPROC_SERVER, // Object "runs" in same process
                                              interface_id,           // ID of interface we request
                                              (void**)&_ptr);         // Returned pointer to interface
            if (!ComSuccess(hr, "CoCreateInstance", report)) {
                _ptr = 0;
            }
            return *this;
        }

        //!
        //! Assign using IUnknown::QueryInterface
        //! If the COM interface is successfully retrieved, it becomes managed and
        //! its reference count is unchanged (== 1).
        //!
        //! @param [in] obj A COM object.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in] report Where to report errors.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& queryInterface(::IUnknown* obj, const IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            assert(obj != 0);
            ::HRESULT hr = obj->QueryInterface(interface_id, (void**)&_ptr);
            if (!ComSuccess(hr, "IUnknown::QueryInterface", report)) {
                _ptr = 0;
            }
            return *this;
        }

        //!
        //! Assign using IMoniker::BindToObject
        //! If the COM interface is successfully retrieved, it becomes managed and
        //! its reference count is unchanged (== 1).
        //!
        //! @param [in] moniker The moniker to use.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in] report Where to report errors.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& bindToObject(::IMoniker* moniker, const IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            assert(moniker != 0);
            ::HRESULT hr = moniker->BindToObject(0,               // No cached context
                                                 0,               // Not part of a composite
                                                 interface_id,    // ID of interface we request
                                                 (void**)&_ptr);  // Returned pointer to interface
            if (!ComSuccess(hr, "IMoniker::BindToObject", report)) {
                _ptr = 0;
            }
            return *this;
        }

        //!
        //! Check if the object exposes an interface.
        //! @param [in] iid Id of the interface we request in the object.
        //! @return True if the object exposes the @a iid interface.
        //!
        bool expose(const ::IID& iid) const
        {
            return ComExpose(_ptr, iid);
        }

        //!
        //! Get the "class name" (formatted GUID) of this object.
        //! Warning: Very slow, eat CPU time, use with care.
        //! @return A formatted GUID or an empty string on error or if the object does not expose IPersist interface.
        //!
        std::string className() const
        {
            ::GUID guid(GUID_NULL);
            ::IPersist* persist;
            if (_ptr != 0 && SUCCEEDED(_ptr->QueryInterface(::IID_IPersist, (void**)&persist))) {
                persist->GetClassID(&guid);
                persist->Release();
            }
            return guid == GUID_NULL ? "" : NameGUID(guid);
        }
    };

    //!
    //! Release all COM objects in a vector (Windows-specific).
    //! Keep vector size (all elements become null pointers).
    //! @tparam COMCLASS A COM interface or object class.
    //! @param [in,out] vec A vector of COM objects to release.
    //!
    template <class COMCLASS>
    void ComVectorRelease(std::vector<ComPtr<COMCLASS>>& vec)
    {
        for (std::vector<ComPtr<COMCLASS>>::iterator it = vec.begin(); it != vec.end(); ++it) {
            it->release();
        }
    }

    //!
    //! Release all COM objects in a vector and clear the vector (Windows-specific).
    //! @tparam COMCLASS A COM interface or object class.
    //! @param [in,out] vec A vector of COM objects to release.
    //!
    template <class COMCLASS>
    void ComVectorClear(std::vector<ComPtr<COMCLASS>>& vec)
    {
        ComVectorRelease(vec);
        vec.clear();
    }
}

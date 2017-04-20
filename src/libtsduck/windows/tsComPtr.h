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

    template <class COMCLASS>
    class ComPtr
    {
    private:
        // Encapsulated pointer to COM object
        COMCLASS* _ptr;

    public:
        // Default constructor -> becomes managed, ref count unchanged
        ComPtr (COMCLASS* p = 0, ::HRESULT hr = S_OK) :
            _ptr (SUCCEEDED (hr) ? p : 0)
        {
        }

        // Copy constructor -> increment ref count
        ComPtr (const ComPtr<COMCLASS>& p) :
            _ptr (p.pointer())
        {
            if (_ptr != 0) {
                _ptr->AddRef();
            }
        }

        // Constructor using CoCreateInstance -> becomes managed, ref count unchanged (== 1)
        ComPtr (const IID& class_id, const IID& interface_id, ReportInterface& report = CERR) :
            _ptr (0)
        {
            createInstance (class_id, interface_id, report);
        }

        // Destructor
        ~ComPtr()
        {
            release();
        }

        // Check if null pointer
        bool isNull() const
        {
            return _ptr == 0;
        }

        // To access COM object, without releasing it
        COMCLASS& operator* () const
        {
            return *_ptr;
        }

        // To access COM object pointer, without releasing it
        COMCLASS* pointer() const
        {
            return _ptr;
        }

        // To access members of COM objects
        COMCLASS* operator-> () const
        {
            return _ptr;
        }

        // Release previous pointer, return a receiver for new pointer.
        // Typically used in ::CoCreateInstance and COM methods returning a new COM interface.
        COMCLASS** creator()
        {
            release();
            return &_ptr;
        }
        
        // Release COM object, pointer becomes null
        void release()
        {
            if (_ptr != 0) {
                _ptr->Release();
                _ptr = 0;
            }
        }

        // Assignment from a ComPtr to a subclass -> increment ref count
        template <class COMSUBCLASS>
        ComPtr<COMCLASS>& assign (const ComPtr<COMSUBCLASS>& p)
        {
            release();
            _ptr = p.pointer();
            if (_ptr != 0) {
                _ptr->AddRef();
            }
            return *this;
        }

        // Assignment between ComPtr to same class -> increment ref count
        ComPtr<COMCLASS>& operator= (const ComPtr<COMCLASS>& p)
        {
            return assign (p);
        }

        // Assignment from a COM object pointer -> becomes managed
        ComPtr<COMCLASS>& operator= (COMCLASS* p)
        {
            release();
            _ptr = p;
            return *this;
        }

        // Assign using CoCreateInstance
        ComPtr<COMCLASS>& createInstance (const IID& class_id, const IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            ::HRESULT hr = ::CoCreateInstance (class_id,               // Class ID for object
                                               NULL,                   // Not part of an aggregate
                                               ::CLSCTX_INPROC_SERVER, // Object "runs" in same process
                                               interface_id,           // ID of interface we request
                                               (void**)&_ptr);         // Returned pointer to interface
            if (!ComSuccess (hr, "CoCreateInstance", report)) {
                _ptr = 0;
            }
            return *this;
        }

        // Assign using IUnknown::QueryInterface
        ComPtr<COMCLASS>& queryInterface (::IUnknown* obj, const IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            assert (obj != 0);
            ::HRESULT hr = obj->QueryInterface (interface_id, (void**)&_ptr);
            if (!ComSuccess (hr, "IUnknown::QueryInterface", report)) {
                _ptr = 0;
            }
            return *this;
        }

        // Assign using IMoniker::BindToObject
        ComPtr<COMCLASS>& bindToObject (::IMoniker* moniker, const IID& interface_id, ReportInterface& report = CERR)
        {
            release();
            assert (moniker != 0);
            ::HRESULT hr = moniker->BindToObject (0,               // No cached context
                                                  0,               // Not part of a composite
                                                  interface_id,    // ID of interface we request
                                                  (void**)&_ptr);  // Returned pointer to interface
            if (!ComSuccess (hr, "IMoniker::BindToObject", report)) {
                _ptr = 0;
            }
            return *this;
        }

        // Check if the object exposes an interface
        bool expose (const IID& iid) const
        {
            ::IUnknown* iface;
            if (_ptr != 0 && SUCCEEDED (_ptr->QueryInterface (iid, (void**)&iface))) {
                iface->Release();
                return true;
            }
            else {
                return false;
            }
        }

        // Return the "class name" (formatted GUID) of this object.
        // Return empty string on error or if the object does not expose IPersist interface.
        // Warning: Very slow, eat CPU time, use with care.
        std::string className() const
        {
            ::GUID guid (GUID_NULL);
            ::IPersist* persist;
            if (_ptr != 0 && SUCCEEDED (_ptr->QueryInterface (::IID_IPersist, (void**)&persist))) {
                persist->GetClassID (&guid);
                persist->Release();
            }
            return guid == GUID_NULL ? "" : NameGUID (guid);
        }
    };

    // Release all COM objects in a vector.
    // Keep vector size (all elements become null pointers).
    template <class COMCLASS>
    void ComVectorRelease (std::vector<ComPtr<COMCLASS>>& vec)
    {
        for (std::vector<ComPtr<COMCLASS>>::iterator it = vec.begin(); it != vec.end(); ++it) {
            it->release();
        }
    }

    // Release all COM objects in a vector then clear vector.
    template <class COMCLASS>
    void ComVectorClear (std::vector<ComPtr<COMCLASS>>& vec)
    {
        ComVectorRelease (vec);
        vec.clear();
    }
}

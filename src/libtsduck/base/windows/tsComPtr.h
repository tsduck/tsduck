//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Managed pointers for COM objects, auto-released (Windows-specific)
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"

#include "tsBeforeStandardHeaders.h"
#include <ObjIdl.h>
#include "tsAfterStandardHeaders.h"

#if defined(DOXYGEN)
    //!
    //! When @c TS_COM_INSTRUMENTATION is externally defined and the application is
    //! compiled in debug mode, some Windows-specific classes using COM objects
    //! produce abundant trace messages on the standard error.
    //!
    #define TS_COM_INSTRUMENTATION 1
#endif

// Make sure that TS_COM_INSTRUMENTATION is undefined in release mode.
// The massive amount of instrumentation messages is reserved to debug.
#if defined(TS_COM_INSTRUMENTATION) && !defined(DEBUG)
    #undef TS_COM_INSTRUMENTATION
#endif

namespace ts {
    //!
    //! Managed pointers for COM objects, auto-released (Windows-specific).
    //! @ingroup windows
    //! @tparam COMCLASS A COM interface or object class.
    //!
    //! If @c TS_COM_INSTRUMENTATION is defined and the application is
    //! compiled in debug mode, the ComPtr class produces abundant trace
    //! messages on the standard error.
    //!
    //! Trace line format:
    //! @code
    //! [COMPTR] comaddr=refcount (@ptraddr): message
    //! @endcode
    //!
    //! With:
    //! - comaddr  : Address of the COM object.
    //! - refcount : Current reference count on the COM object after the operation.
    //! - ptraddr  : Address of the ComPtr object.
    //!
    template <class COMCLASS>
    class ComPtr
    {
    private:
#if defined(TS_COM_INSTRUMENTATION)
        mutable bool _traceCreator;  // A call to creator() has returned &_ptr and the resulting pointer was not yet traced.
#endif
        COMCLASS* _ptr;  // Encapsulated pointer to COM object.

    public:
        //!
        //! Default constructor.
        //! @param [in] p Address of a COM object. If @a p is not null and @a hr is success,
        //! the COM object becomes managed by ComPtr. Its reference count is unchanged.
        //! @param [in] hr An @c HRESULT value, typically the returned status of the system
        //! call which created @a p.
        //!
        ComPtr(COMCLASS* p = 0, ::HRESULT hr = S_OK);

        //!
        //! Copy constructor.
        //! The reference count of the COM object is incremented.
        //! @param [in] p Another ComPtr instance.
        //!
        ComPtr(const ComPtr<COMCLASS>& p);

        //!
        //! Move constructor.
        //! The reference count of the COM object is not incremented.
        //! @param [in,out] p Another ComPtr instance. Cleared on output.
        //!
        ComPtr(ComPtr<COMCLASS>&& p);

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
        ComPtr(const ::IID& class_id, const ::IID& interface_id, Report& report = CERR);

        //!
        //! Constructor using IUnknown::QueryInterface().
        //! If the COM interface is successfully retrieved, it becomes managed and
        //! its reference count is unchanged (== 1).
        //!
        //! @param [in] obj A COM object.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in] report Where to report errors.
        //!
        ComPtr(::IUnknown* obj, const IID& interface_id, Report& report = CERR);

        //!
        //! Destructor.
        //! The COM object is released (its reference count is decremented).
        //!
        ~ComPtr();

        //!
        //! Check if null pointer.
        //! @return True if this is a null pointer (no object).
        //!
        bool isNull() const;

        //!
        //! To access a COM object pointer, without releasing it.
        //! @return A pointer to the COM object.
        //!
        COMCLASS* pointer() const;

        //!
        //! Dereference operator.
        //! To access a COM object.
        //! @return A reference to the COM object.
        //!
        COMCLASS& operator*() const;

        //!
        //! Dereference operator.
        //! To access members of COM objects.
        //! @return A pointer to the COM object.
        //!
        COMCLASS* operator->() const;

        //!
        //! Release previous pointer, return a receiver for new pointer.
        //! Typically used in CoCreateInstance() and COM methods returning a new COM interface.
        //! @return The address of the internal COM object pointer.
        //!
        COMCLASS** creator();

        //!
        //! Release the COM object, its reference count is decremented.
        //! The pointer becomes null.
        //!
        void release();

        //!
        //! Get the reference count to the object.
        //! WARNING: This should be used for test or debug purpose only.
        //! Since there is no direct way to get the reference count of a COM object,
        //! we perform an AddRef / Release operation. The reference count is returned by Release().
        //! @return The reference count for the object or zero if this is a null pointer.
        //!
        int refCount() const;

        //!
        //! Assignment from a ComPtr to a subclass.
        //! The reference count of the COM object is incremented.
        //! @tparam COMSUBCLASS A COM interface or object class, a subclass of @a COMCLASS.
        //! @param [in] p A ComPtr to a @a COMSUBCLASS object.
        //! @return A reference to this object.
        //!
        template <class COMSUBCLASS, typename std::enable_if<std::is_base_of<COMCLASS,COMSUBCLASS>::value>::type* = nullptr>
        ComPtr<COMCLASS>& assign(const ComPtr<COMSUBCLASS>& p);

        //!
        //! Assignment operator.
        //! The reference count of the COM object is incremented.
        //! @param [in] p A ComPtr to a COM object.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& operator=(const ComPtr<COMCLASS>& p);

        //!
        //! Move assignment operator.
        //! The reference count of the COM object is not incremented.
        //! @param [in,out] p A ComPtr to a COM object. Cleared on return.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& operator=(ComPtr<COMCLASS>&& p);

        //!
        //! Assignment operator from a COM object pointer.
        //! The COM object becomes managed. Its reference count is unchanged.
        //! @param [in] p Address of a COM object. Can be null.
        //! @return A reference to this object.
        //!
        ComPtr<COMCLASS>& operator=(COMCLASS* p);

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
        ComPtr<COMCLASS>& createInstance(const ::IID& class_id, const ::IID& interface_id, Report& report = CERR);

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
        ComPtr<COMCLASS>& queryInterface(::IUnknown* obj, const IID& interface_id, Report& report = CERR);

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
        ComPtr<COMCLASS>& bindToObject(::IMoniker* moniker, const IID& interface_id, Report& report = CERR);

        //!
        //! Check if the object exposes an interface.
        //! @param [in] iid Id of the interface we request in the object.
        //! @return True if the object exposes the @a iid interface.
        //!
        bool expose(const ::IID& iid) const;

        //!
        //! Get the "class name" (formatted GUID) of this object.
        //! Warning: Very slow, eat CPU time, use with care.
        //! @return A formatted GUID or an empty string on error or if the object does not expose IPersist interface.
        //!
        UString className() const;
    };
}

#include "tsComPtrTemplate.h"

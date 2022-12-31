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

#pragma once
#include "tsWinUtils.h"


//-----------------------------------------------------------------------------
// Instrumentation macros.
//-----------------------------------------------------------------------------

#if defined(TS_COM_INSTRUMENTATION)

#define TRACE_HEADER(adj) std::cerr << ts::UString::Format(u"[COMPTR] %X=%-3d(@%X): ", size_t(_ptr), refCount() + (adj), size_t(this))
#define TRACE_TRAILER()   std::endl << std::flush

#define TRACE_CONSTRUCT() (_ptr != nullptr ? (TRACE_HEADER(0) << "constructor" << TRACE_TRAILER()) : std::cerr)
#define TRACE_COCREATE()  (_ptr != nullptr ? (TRACE_HEADER(0) << "CoCreateInstance" << TRACE_TRAILER()) : std::cerr)
#define TRACE_QUERY()     (_ptr != nullptr ? (TRACE_HEADER(0) << "QueryInterface" << TRACE_TRAILER()) : std::cerr)
#define TRACE_BIND()      (_ptr != nullptr ? (TRACE_HEADER(0) << "BindToObject" << TRACE_TRAILER()) : std::cerr)
#define TRACE_CREATOR()   (_traceCreator = true)
#define TRACE_ENTRY()     (_traceCreator ? ((_ptr != nullptr ? (TRACE_HEADER(0) << "creator" << TRACE_TRAILER()) : std::cerr), _traceCreator = false) : false)
#define TRACE_ADDREF()    (TRACE_HEADER(0) << "AddRef" << TRACE_TRAILER())
#define TRACE_RELEASE()   (TRACE_HEADER(-1) << "Release" << TRACE_TRAILER())  // Must be traced before calling Release() => adjust refcount by -1.

#else

#define TRACE_CONSTRUCT()
#define TRACE_COCREATE()
#define TRACE_QUERY()
#define TRACE_BIND()
#define TRACE_CREATOR()
#define TRACE_ENTRY()
#define TRACE_ADDREF()
#define TRACE_RELEASE()

#endif


//-----------------------------------------------------------------------------
// Default constructor.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(COMCLASS* p, ::HRESULT hr) :

#if defined(TS_COM_INSTRUMENTATION)
    _traceCreator(false),
#endif
    _ptr(SUCCEEDED(hr) ? p : nullptr)
{
    TRACE_CONSTRUCT();
}


//-----------------------------------------------------------------------------
// Copy constructor.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(const ComPtr<COMCLASS>& p) :

#if defined(TS_COM_INSTRUMENTATION)
    _traceCreator(false),
#endif
    _ptr(p.pointer())
{
    if (_ptr != nullptr) {
        _ptr->AddRef();
        TRACE_ADDREF();
    }
}


//-----------------------------------------------------------------------------
// Move constructor.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(ComPtr<COMCLASS>&& p) :

#if defined(TS_COM_INSTRUMENTATION)
    _traceCreator(false),
#endif
    _ptr(p.pointer())
{
    p._ptr = nullptr;
    TRACE_CONSTRUCT();
}


//-----------------------------------------------------------------------------
// Constructor using CoCreateInstance().
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(const ::IID& class_id, const ::IID& interface_id, Report& report) :

#if defined(TS_COM_INSTRUMENTATION)
    _traceCreator(false),
#endif
    _ptr(nullptr)
{
    createInstance(class_id, interface_id, report);
}


//-----------------------------------------------------------------------------
// Constructor using IUnknown::QueryInterface().
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(::IUnknown* obj, const IID& interface_id, Report& report) :

#if defined(TS_COM_INSTRUMENTATION)
    _traceCreator(false),
#endif
    _ptr(nullptr)
{
    queryInterface(obj, interface_id, report);
}


//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::~ComPtr()
{
    release();
}


//-----------------------------------------------------------------------------
// Check if null pointer.
//-----------------------------------------------------------------------------

template <class COMCLASS>
bool ts::ComPtr<COMCLASS>::isNull() const
{
    TRACE_ENTRY();
    return _ptr == nullptr;
}


//-----------------------------------------------------------------------------
// Dereference operator.
//-----------------------------------------------------------------------------

template <class COMCLASS>
COMCLASS& ts::ComPtr<COMCLASS>::operator*() const
{
    TRACE_ENTRY();
    return *_ptr;
}


//-----------------------------------------------------------------------------
// To access a COM object pointer, without releasing it.
//-----------------------------------------------------------------------------

template <class COMCLASS>
COMCLASS* ts::ComPtr<COMCLASS>::pointer() const
{
    TRACE_ENTRY();
    return _ptr;
}


//-----------------------------------------------------------------------------
// Dereference operator.
//-----------------------------------------------------------------------------

template <class COMCLASS>
COMCLASS* ts::ComPtr<COMCLASS>::operator->() const
{
    TRACE_ENTRY();
    return _ptr;
}


//-----------------------------------------------------------------------------
// Release previous pointer, return a receiver for new pointer.
//-----------------------------------------------------------------------------

template <class COMCLASS>
COMCLASS** ts::ComPtr<COMCLASS>::creator()
{
    release();
    TRACE_CREATOR();
    return &_ptr;
}


//-----------------------------------------------------------------------------
// Release the COM object, its reference count is decremented.
//-----------------------------------------------------------------------------

template <class COMCLASS>
void ts::ComPtr<COMCLASS>::release()
{
    TRACE_ENTRY();
    if (_ptr != nullptr) {
        TRACE_RELEASE();
        _ptr->Release();
        _ptr = 0;
    }
}


//-----------------------------------------------------------------------------
// Get the reference count to the object.
//-----------------------------------------------------------------------------

template <class COMCLASS>
int ts::ComPtr<COMCLASS>::refCount() const
{
    if (_ptr == nullptr) {
        return 0;
    }
    else {
        // No trace macro here, would recurse...
        _ptr->AddRef();
        return int(_ptr->Release());
    }
}


//-----------------------------------------------------------------------------
// Assignment from a ComPtr to a subclass.
//-----------------------------------------------------------------------------

template <class COMCLASS>
template <class COMSUBCLASS, typename std::enable_if<std::is_base_of<COMCLASS,COMSUBCLASS>::value>::type*>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::assign(const ComPtr<COMSUBCLASS>& p)
{
    TRACE_ENTRY();
    // Do not do anything if the two ComPtr already point to the same COM object.
    // This also exclude two null pointers and self-assignment.
    if (_ptr != p.pointer()) {
        release();
        _ptr = p.pointer();
        if (_ptr != nullptr) {
            _ptr->AddRef();
            TRACE_ADDREF();
        }
    }
    return *this;
}


//-----------------------------------------------------------------------------
// Assignment operator.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::operator=(const ComPtr<COMCLASS>& p)
{
    return assign(p);
}


//-----------------------------------------------------------------------------
// Move assignment operator.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::operator=(ComPtr<COMCLASS>&& p)
{
    TRACE_ENTRY();
    // Do not do anything if the two ComPtr already point to the same COM object.
    // This also exclude two null pointers and self-assignment.
    if (_ptr != p.pointer()) {
        release();
        _ptr = p.pointer();
        p._ptr = nullptr;
    }
    return *this;
}


//-----------------------------------------------------------------------------
// Assignment operator from a COM object pointer.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::operator=(COMCLASS* p)
{
    release();
    _ptr = p;
    return *this;
}


//-----------------------------------------------------------------------------
// Assign using CoCreateInstance().
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::createInstance(const ::IID& class_id, const ::IID& interface_id, Report& report)
{
    release();
    ::HRESULT hr = ::CoCreateInstance(class_id,               // Class ID for object
                                      NULL,                   // Not part of an aggregate
                                      ::CLSCTX_INPROC_SERVER, // Object "runs" in same process
                                      interface_id,           // ID of interface we request
                                      (void**)&_ptr);         // Returned pointer to interface
    if (!ComSuccess(hr, u"CoCreateInstance", report)) {
        _ptr = nullptr;
    }
    TRACE_COCREATE();
    return *this;
}


//-----------------------------------------------------------------------------
// Assign using IUnknown::QueryInterface
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::queryInterface(::IUnknown* obj, const IID& interface_id, Report& report)
{
    release();
    if (obj != 0) {
        ::HRESULT hr = obj->QueryInterface(interface_id, (void**)&_ptr);
        if (!ComSuccess(hr, u"IUnknown::QueryInterface", report)) {
            _ptr = nullptr;
        }
        TRACE_QUERY();
    }
    return *this;
}


//-----------------------------------------------------------------------------
// Assign using IMoniker::BindToObject
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>& ts::ComPtr<COMCLASS>::bindToObject(::IMoniker* moniker, const IID& interface_id, Report& report)
{
    release();
    if (moniker != nullptr) {
        ::HRESULT hr = moniker->BindToObject(nullptr,         // No cached context
                                             nullptr,         // Not part of a composite
                                             interface_id,    // ID of interface we request
                                             (void**)&_ptr);  // Returned pointer to interface
        if (!ComSuccess(hr, u"IMoniker::BindToObject", report)) {
            _ptr = nullptr;
        }
        TRACE_BIND();
    }
    return *this;
}


//-----------------------------------------------------------------------------
// Check if the object exposes an interface.
//-----------------------------------------------------------------------------

template <class COMCLASS>
bool ts::ComPtr<COMCLASS>::expose(const ::IID& iid) const
{
    TRACE_ENTRY();
    return ComExpose(_ptr, iid);
}


//-----------------------------------------------------------------------------
// Get the "class name" (formatted GUID) of this object.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::UString ts::ComPtr<COMCLASS>::className() const
{
    TRACE_ENTRY();
    ::GUID guid(GUID_NULL);
    ::IPersist* persist = nullptr;
    if (_ptr != nullptr && SUCCEEDED(_ptr->QueryInterface(::IID_IPersist, (void**)&persist))) {
        persist->GetClassID(&guid);
        persist->Release();
    }
    return guid == GUID_NULL ? UString() : NameGUID(guid);
}

#undef TRACE_CONSTRUCT
#undef TRACE_COCREATE
#undef TRACE_QUERY
#undef TRACE_BIND
#undef TRACE_CREATOR
#undef TRACE_ENTRY
#undef TRACE_ADDREF
#undef TRACE_RELEASE

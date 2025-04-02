//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Managed pointers for COM objects, auto-released (Windows-specific)
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsWinUtils.h"

#include "tsBeforeStandardHeaders.h"
#include <objidl.h>
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
    //! @ingroup libtscore windows
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
        mutable bool _traceCreator = false;  // A call to creator() has returned &_ptr and the resulting pointer was not yet traced.
#endif
        COMCLASS* _ptr = nullptr;  // Encapsulated pointer to COM object.

    public:
        //!
        //! Default constructor.
        //! @param [in] p Address of a COM object. If @a p is not null and @a hr is success,
        //! the COM object becomes managed by ComPtr. Its reference count is unchanged.
        //! @param [in] hr An @c HRESULT value, typically the returned status of the system
        //! call which created @a p.
        //!
        ComPtr(COMCLASS* p = nullptr, ::HRESULT hr = S_OK);

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
        template <class COMSUBCLASS> requires std::derived_from<COMSUBCLASS, COMCLASS>
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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Instrumentation macros.
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
// Constructors and destructor.
//-----------------------------------------------------------------------------

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(COMCLASS* p, ::HRESULT hr) :
    _ptr(SUCCEEDED(hr) ? p : nullptr)
{
    TRACE_CONSTRUCT();
}

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(const ComPtr<COMCLASS>& p) :
    _ptr(p.pointer())
{
    if (_ptr != nullptr) {
        _ptr->AddRef();
        TRACE_ADDREF();
    }
}

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(ComPtr<COMCLASS>&& p) :
    _ptr(p.pointer())
{
    p._ptr = nullptr;
    TRACE_CONSTRUCT();
}

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(const ::IID& class_id, const ::IID& interface_id, Report& report)
{
    createInstance(class_id, interface_id, report);
}

template <class COMCLASS>
ts::ComPtr<COMCLASS>::ComPtr(::IUnknown* obj, const IID& interface_id, Report& report)
{
    queryInterface(obj, interface_id, report);
}

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
        _ptr = nullptr;
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
template <class COMSUBCLASS> requires std::derived_from<COMSUBCLASS, COMCLASS>
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
                                      nullptr,                // Not part of an aggregate
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
    if (obj != nullptr) {
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

#endif // DOXYGEN

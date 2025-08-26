//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtscore windows
//!  Utilities for Windows and Common Object Model (COM).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"
#include "tsComIds.h"

#include "tsBeforeStandardHeaders.h"
#include <objidl.h>
#include "tsAfterStandardHeaders.h"

namespace ts {
    //!
    //! Standard Windows language code for US-English.
    //!
    constexpr ::DWORD US_ENGLISH_CODE = 0x0409;

    //!
    //! Format a Windows error message (Windows-specific).
    //! @param [in] code An error status code.
    //! @return The corresponding message string.
    //!
    TSCOREDLL UString WinErrorMessage(::DWORD code);

    //!
    //! Format the message for a COM status (Windows-specific).
    //! @param [in] status A COM status.
    //! @return The corresponding message string.
    //!
    TSCOREDLL UString ComMessage(::HRESULT status);

    //!
    //! Check a COM status (Windows-specific).
    //! In case of error, report an error message.
    //! @param [in] status A COM status.
    //! @param [in] message Application message in case of error.
    //! The COM message is appended to the application message.
    //! @param [in,out] report Where to report errors.
    //! @return True if status is success, false if error.
    //!
    TSCOREDLL bool ComSuccess(::HRESULT status, const UChar* message, Report& report);

    //!
    //! Check a COM status (Windows-specific).
    //! In case of error, report an error message.
    //! @param [in] status A COM status.
    //! @param [in] message Application message in case of error.
    //! The COM message is appended to the application message.
    //! @param [in,out] report Where to report errors.
    //! @return True if status is success, false if error.
    //!
    TSCOREDLL bool ComSuccess(::HRESULT status, const UString& message, Report& report);

    //!
    //! Check if a COM object exposes an interface (Windows-specific).
    //! @param [in] object Object to query.
    //! @param [in] iid Id of the interface we request in the object.
    //! @return True if @a object exposes the @a iid interface.
    //!
    TSCOREDLL bool ComExpose(::IUnknown* object, const ::IID& iid);

    //!
    //! Convert a COM string to Unicode string (Windows-specific).
    //! @param [in] s The COM string.
    //! @return The equivalent C++ string or an empty string on error.
    //!
    TSCOREDLL UString ToString(const ::VARIANT& s);

    //!
    //! Convert a Windows string to Unicode string (Windows-specific).
    //! @param [in] s The COM string.
    //! @return The equivalent C++ string or an empty string on error.
    //!
    TSCOREDLL UString ToString(const ::BSTR s);

    //!
    //! Convert a Windows string to Unicode string (Windows-specific).
    //! @param [in] s The Windows string.
    //! @return The equivalent Unicode string or an empty string on error.
    //!
    TSCOREDLL UString ToString(const ::WCHAR* s);

    //!
    //! Format a GUID as string (Windows-specific).
    //! @param [in] guid A GUID.
    //! @param [in] with_braces If true, add the surrounding braces "{...}".
    //! @return The equivalent string or an empty string on error.
    //!
    TSCOREDLL UString FormatGUID(const ::GUID& guid, bool with_braces = true);

    //!
    //! Get a "canonical" version of a GUID (Windows-specific).
    //! @param [in] guid A GUID.
    //! @return The equivalent string with only lower-case hexa digits.
    //!
    TSCOREDLL UString CanonicalGUID(const ::GUID& guid);

    //!
    //! Get a "canonical" version of a GUID string (Windows-specific).
    //! @param [in] guid A GUID string.
    //! @return The equivalent string with only lower-case hexa digits.
    //!
    TSCOREDLL UString CanonicalGUID(const UString& guid);

    //!
    //! Format a GUID as string and resolve a few known names (Windows-specific).
    //! Warning: Very slow, eat CPU time, use with care.
    //! @param [in] guid A GUID.
    //! @return The equivalent string or an empty string on error.
    //!
    TSCOREDLL UString NameGUID(const ::GUID& guid);

    //!
    //! Get a string property from the "property bag" of a COM object (Windows-specific).
    //! @param [in,out] moniker Moniker defining the object.
    //! @param [in] property_name Name of the property to fetch.
    //! @param [in,out] report Where to report errors.
    //! @return The property value.
    //!
    TSCOREDLL UString GetStringPropertyBag(::IMoniker* moniker, const ::OLECHAR* property_name, Report& report);

    //!
    //! Get the handle of a COM object (Windows-specific).
    //! @param [in] obj COM object.
    //! @param [in,out] report Where to report errors.
    //! @return The handle or INVALID_HANDLE_VALUE on error.
    //!
    TSCOREDLL ::HANDLE GetHandleFromObject(::IUnknown* obj, Report& report);

    //!
    //! Get the device or file name from a Windows handle (Windows-specific).
    //! @param [in] handle A Windows handle.
    //! @return The device name or an empty string on error.
    //!
    TSCOREDLL UString WinDeviceName(::HANDLE handle);

    //!
    //! Start an application with elevated privileges (Windows-specific).
    //! @param [in] exeName Path name of the executable file to run.
    //! @param [in] synchronous If true, wait for the process to terminate.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool WinCreateElevatedProcess(const UString& exeName, bool synchronous, Report& report);
}

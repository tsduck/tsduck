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
//!  Utilities for Windows Common Object Model (COM).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsComIds.h"

namespace ts {

    //!
    //! Format the message for a COM status (Windows-specific).
    //! @param [in] status A COM status.
    //! @return The corresponding message string.
    //!
    TSDUCKDLL std::string ComMessage(::HRESULT status);

    //!
    //! Check a COM status (Windows-specific).
    //! In case of error, report an error message.
    //! @param [in] status A COM status.
    //! @param [in] message Application message in case of error.
    //! The COM message is appended to the application message.
    //! @param [in,out] report Where to report errors.
    //! @return True if status is success, false if error.
    //!
    TSDUCKDLL bool ComSuccess(::HRESULT status, const char* message, Report& report);

    //!
    //! Check a COM status (Windows-specific).
    //! In case of error, report an error message.
    //! @param [in] status A COM status.
    //! @param [in] message Application message in case of error.
    //! The COM message is appended to the application message.
    //! @param [in,out] report Where to report errors.
    //! @return True if status is success, false if error.
    //!
    TSDUCKDLL bool ComSuccess(::HRESULT status, const std::string& message, Report& report);

    //!
    //! Check if an object exposes an interface.
    //! @param [in] object Object to query.
    //! @param [in] iid Id of the interface we request in the object.
    //! @return True if @a object exposes the @a iid interface.
    //!
    TSDUCKDLL bool ComExpose(::IUnknown* object, const ::IID& iid);

    //!
    //! Convert a COM string to std::string (Windows-specific).
    //! @param [in] s The COM string.
    //! @return The equivalent C++ string or an empty string on error.
    //!
    TSDUCKDLL std::string ToString(const ::VARIANT& s);

    //!
    //! Convert a COM string to std::string (Windows-specific).
    //! @param [in] s The COM string.
    //! @return The equivalent C++ string or an empty string on error.
    //!
    TSDUCKDLL std::string ToString(const ::BSTR s);

    //!
    //! Convert a Unicode string to std::string (Windows-specific).
    //! @param [in] s The Unicode string.
    //! @return The equivalent C++ string or an empty string on error.
    //!
    TSDUCKDLL std::string ToString(const ::WCHAR* s);

    //!
    //! Format a GUID as string (Windows-specific).
    //! @param [in] guid A GUID.
    //! @param [in] with_braces If true, add the surrounding braces "{...}".
    //! @return The equivalent string or an empty string on error.
    //!
    TSDUCKDLL std::string FormatGUID(const ::GUID& guid, bool with_braces = true);

    //!
    //! Format a GUID as string and resolve a few known names (Windows-specific).
    //! Warning: Very slow, eat CPU time, use with care.
    //! @param [in] guid A GUID.
    //! @return The equivalent string or an empty string on error.
    //!
    TSDUCKDLL std::string NameGUID(const ::GUID& guid);

    //!
    //! Get a string property from the "property bag" of an object (Windows-specific).
    //! @param [in,out] moniker Moniker defining the object.
    //! @param [in] property_name Name of the property to fetch.
    //! @param [in,out] report Where to report errors.
    //! @return The property value.
    //!
    TSDUCKDLL std::string GetStringPropertyBag(::IMoniker* moniker, const ::OLECHAR* property_name, Report& report);
}

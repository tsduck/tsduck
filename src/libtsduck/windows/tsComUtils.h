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
#include "tsReportInterface.h"
#include "tsComIds.h"

namespace ts {

    // Format the message for a COM status
    TSDUCKDLL std::string ComMessage (::HRESULT);

    // Check a COM status. In case of error, report an error message.
    // Return true if status is success, false if error.
    TSDUCKDLL bool ComSuccess (::HRESULT, const char* message, ReportInterface&);
    TSDUCKDLL bool ComSuccess (::HRESULT, const std::string& message, ReportInterface&);

    // Convert COM and Unicode strings to std::string (empty on error)
    TSDUCKDLL std::string ToString (const ::VARIANT&);
    TSDUCKDLL std::string ToString (const ::BSTR);
    TSDUCKDLL std::string ToString (const ::WCHAR*);

    // Format a GUID as string.
    TSDUCKDLL std::string FormatGUID (const ::GUID&, bool with_braces = true);

    // Format the name of a GUID. Resolve a few known names.
    // Warning: Very slow, eat CPU time, use with care.
    TSDUCKDLL std::string NameGUID (const ::GUID&);

    // Return a string property from the "property bag" of an object
    // (defined by an object moniker)
    TSDUCKDLL std::string GetStringPropertyBag (::IMoniker*, const ::OLECHAR* property_name, ReportInterface&);
}

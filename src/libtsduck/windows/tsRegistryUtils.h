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
//!  Windows Registry utilities (Windows-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReportInterface.h"

namespace ts {

    // Vocabulary :
    //   Key        : Node of the registry (kind of "directory")
    //   Value_Name : Name of a value in a key
    //   Value      : Value of the Value_Name

    // Get a value in a registry key as a string.
    // Return an empty string if non-existent or error.
    TSDUCKDLL std::string GetRegistryValue (const std::string& key, const std::string& value_name = "");

    // Set value of a registry key.
    // If expandable is true, set the type to REG_EXPAND_SZ.
    // Otherwise, set type to REG_SZ.
    // Return true on success, false on error.
    TSDUCKDLL bool SetRegistryValue (const std::string& key, const std::string& value_name, const std::string& value, bool expandable = false);

    // Set value of a registry key.
    // Set the data type as REG_DWORD.
    // Return true on success, false on error.
    TSDUCKDLL bool SetRegistryValue (const std::string& key, const std::string& value_name, ::DWORD value);

    // Delete a value of a registry key.
    // Return true on success, false on error.
    TSDUCKDLL bool DeleteRegistryValue (const std::string& key, const std::string& value_name);

    // Create a registry key.
    // Return true on success, false on error.
    TSDUCKDLL bool CreateRegistryKey (const std::string& key, bool is_volatile = false);

    // Delete a registry key.
    // Return true on success, false on error.
    TSDUCKDLL bool DeleteRegistryKey (const std::string& key);

    // Notify all applications of a setting change.
    // Return true on success, false on error.
    TSDUCKDLL bool NotifySettingChange();
}

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
//!  Vocabulary :
//!  - Key        : Node of the registry (kind of "directory").
//!  - Value_Name : Name of a value in a key.
//!  - Value      : Value of the Value_Name.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Get a value in a registry key as a string (Windows-specific).
    //! @param [in] key Registry key.
    //! @param [in] value_name Name of the value in @a key.
    //! @return An empty string if non-existent or error.
    //!
    TSDUCKDLL UString GetRegistryValue(const UString& key, const UString& value_name = UString());

    //!
    //! Set the value of a registry key (Windows-specific).
    //! @param [in] key Registry key.
    //! @param [in] value_name Name of the value in @a key.
    //! @param [in] value Value to set in @a value_name.
    //! @param [in] expandable If true, set the type to REG_EXPAND_SZ.
    //! Otherwise, set type to REG_SZ.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SetRegistryValue(const UString& key,
                                    const UString& value_name,
                                    const UString& value,
                                    bool expandable = false);

    //!
    //! Set value of a registry key (Windows-specific).
    //! Set the data type as REG_DWORD.
    //! @param [in] key Registry key.
    //! @param [in] value_name Name of the value in @a key.
    //! @param [in] value Value to set in @a value_name.
    //! Set the data type as REG_DWORD.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SetRegistryValue(const UString& key, const UString& value_name, ::DWORD value);

    //!
    //! Delete a value of a registry key (Windows-specific).
    //! @param [in] key Registry key.
    //! @param [in] value_name Name of the value in @a key.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteRegistryValue(const UString& key, const UString& value_name);

    //!
    //! Create a registry key (Windows-specific).
    //! @param [in] key Registry key to create.
    //! @param [in] is_volatile If true, create a "volatile" registry key.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateRegistryKey(const UString& key, bool is_volatile = false);

    //!
    //! Delete a registry key (Windows-specific).
    //! @param [in] key Registry key to delete.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteRegistryKey(const UString& key);

    //!
    //! Get the root key of a registry path.
    //! @param [in] key Registry key to split.
    //! @param [out] root_key Handle to the root key.
    //! @param [out] subkey Subkey name.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SplitRegistryKey(const UString& key, ::HKEY& root_key, UString& subkey);

    //!
    //! Get the root key of a registry path.
    //! @param [in] key Registry key to split.
    //! @param [out] root_key Handle to the root key.
    //! @param [out] midkey Middle key name (without root key and final component).
    //! @param [out] final_key Final component of the key.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SplitRegistryKey(const UString& key, ::HKEY& root_key, UString& midkey, UString& final_key);

    //!
    //! Notify all applications of a setting change (Windows-specific).
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool NotifySettingChange();
}

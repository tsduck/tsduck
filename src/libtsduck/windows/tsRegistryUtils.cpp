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
//
//  Windows Registry utilities. Windows-specific.
//
//-----------------------------------------------------------------------------

#include "tsRegistryUtils.h"
#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsFormat.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Internal functions: return the root key of a registry path.
//-----------------------------------------------------------------------------

namespace {
    bool SplitKey (const std::string& key, ::HKEY& root_key, std::string& subkey)
    {
        using ts::SimilarStrings;

        size_t sep = key.find ('\\');
        std::string root (key.substr (0, sep == std::string::npos ? key.length() : sep));
        subkey = sep == std::string::npos ? "" : key.substr (sep + 1);

        if (SimilarStrings (root, "HKEY_CLASSES_ROOT") || SimilarStrings (root, "HKCR")) {
            root_key = HKEY_CLASSES_ROOT;
        }
        else if (SimilarStrings (root, "HKEY_CURRENT_USER") || SimilarStrings (root, "HKCU")) {
            root_key = HKEY_CURRENT_USER;
        }
        else if (SimilarStrings (root, "HKEY_LOCAL_MACHINE") || SimilarStrings (root, "HKLM")) {
            root_key = HKEY_LOCAL_MACHINE;
        }
        else if (SimilarStrings (root, "HKEY_USERS") || SimilarStrings (root, "HKU")) {
            root_key = HKEY_USERS;
        }
        else if (SimilarStrings (root, "HKEY_CURRENT_CONFIG") || SimilarStrings (root, "HKCC")) {
            root_key = HKEY_CURRENT_CONFIG;
        }
        else if (SimilarStrings (root, "HKEY_PERFORMANCE_DATA") || SimilarStrings (root, "HKPD")) {
            root_key = HKEY_PERFORMANCE_DATA;
        }
        else {
            root_key = NULL;
            return false;
        }
        return true;
    }

    bool SplitKey (const std::string& key, ::HKEY& root_key, std::string& midkey, std::string& final_key)
    {
        midkey.clear();
        bool ok = SplitKey (key, root_key, final_key);
        if (ok) {
            size_t sep = final_key.rfind ('\\');
            if (sep != std::string::npos) {
                midkey = final_key.substr (0, sep);
                final_key.erase (0, sep + 1);
            }
        }
        return ok;
    }
}


//-----------------------------------------------------------------------------
// Get a value in a registry key as a string.
// Return an empty string if non-existent or error.
//-----------------------------------------------------------------------------

std::string ts::GetRegistryValue (const std::string& key, const std::string& value_name)
{
    // Split name
    ::HKEY root;
    std::string subkey;
    if (!SplitKey (key, root, subkey)) {
        return "";
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, subkey.c_str(), 0, KEY_READ, &hkey);
    if (hr != ERROR_SUCCESS) {
        return "";
    }

    // Query the the size of the value in the key. By giving a NULL address
    // to lpData, RegQueryValueEx simply returns the size of the value.
    ::DWORD type;
    ::DWORD size = 0;
    hr = ::RegQueryValueEx (hkey, value_name.c_str(), NULL, &type, NULL, &size);
    if ((hr != ERROR_SUCCESS && hr != ERROR_MORE_DATA) || size <= 0) {
        ::RegCloseKey (hkey);
        return "";
    }

    // Allocate new buffer and actually get the value
    ::DWORD bufsize = (size < 0 ? 0 : size) + 10;
    ::BYTE* buf = new ::BYTE[size = bufsize];
    CheckNonNull (buf);
    hr = ::RegQueryValueEx (hkey, value_name.c_str(), NULL, &type, buf, &size);
    ::RegCloseKey (hkey);
    if (hr != ERROR_SUCCESS) {
        size = 0;
        type = REG_NONE;
    }
    size = std::max (::DWORD(0), std::min (size, bufsize - 1));
    buf[size] = 0; // if improperly terminated string

    // Convert value to a string
    std::string value;
    switch (type) {
        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
            // There is at least one nul-terminated string in. If the type is REG_MULTI_SZ,
            // there are several nul-terminated strings, ending with two nuls, but we keep only the first string.
            value = reinterpret_cast<char*>(buf);
            break;
        case REG_DWORD:
            value = Format ("%ld", long (*reinterpret_cast<::DWORD*>(buf)));
            break;
        case REG_DWORD_BIG_ENDIAN:
            value = Format ("%ld", long (GetUInt32 (buf)));
            break;
    }

    // Return value
    delete[] buf;
    return value;
}


//-----------------------------------------------------------------------------
// Set value of a registry key.
// If expandable is true, set the type to REG_EXPAND_SZ.
// Otherwise, set type to REG_SZ.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::SetRegistryValue (const std::string& key, const std::string& value_name, const std::string& value, bool expandable)
{
    // Split name
    ::HKEY root;
    std::string subkey;
    if (!SplitKey (key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, subkey.c_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Set the value
    hr = ::RegSetValueEx (hkey,
                          value_name.c_str(),
                          0, // reserved
                          expandable ? REG_EXPAND_SZ : REG_SZ,
                          reinterpret_cast<const ::BYTE*> (value.c_str()),
                          ::DWORD(value.length() + 1)); // include terminating nul

    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey (hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Set value of a registry key.
// Set the data type as REG_DWORD.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool SetRegistryValue (const std::string& key, const std::string& value_name, ::DWORD value)
{
    // Split name
    ::HKEY root;
    std::string subkey;
    if (!SplitKey (key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, subkey.c_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Set the value
    hr = ::RegSetValueEx (hkey,
                          value_name.c_str(),
                          0, // reserved
                          REG_DWORD,
                          reinterpret_cast<const ::BYTE*> (&value),
                          sizeof (value));

    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey (hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a value of a registry key.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool DeleteRegistryValue (const std::string& key, const std::string& value_name)
{
    // Split name
    ::HKEY root;
    std::string subkey;
    if (!SplitKey (key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, subkey.c_str(), 0, KEY_SET_VALUE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Delete the value
    hr = ::RegDeleteValue (hkey, value_name.c_str());
    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey (hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Create a registry key.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool CreateRegistryKey (const std::string& key, bool is_volatile)
{
    // Split name
    ::HKEY root;
    std::string midkey, newkey;
    if (!SplitKey (key, root, midkey, newkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, midkey.c_str(), 0, KEY_CREATE_SUB_KEY | KEY_READ, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Create the key
    ::HKEY hnewkey;
    hr = ::RegCreateKeyEx (hkey,
                           newkey.c_str(),
                           0,  // reserved
                           NULL, // class
                           is_volatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                           0, // security: no further access
                           NULL, // security attributes
                           &hnewkey,
                           NULL); // disposition

    bool success = hr == ERROR_SUCCESS;
    if (success) {
        ::RegCloseKey (hnewkey);
    }
    ::RegCloseKey (hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a registry key.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool DeleteRegistryKey (const std::string& key)
{
    // Split name
    ::HKEY root;
    std::string midkey, newkey;
    if (!SplitKey (key, root, midkey, newkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyEx (root, midkey.c_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Delete the key
    hr = ::RegDeleteKey (hkey, newkey.c_str());
    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey (hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Notify all applications of a setting change.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::NotifySettingChange()
{
    // timeout: 5000 ms
    ::LRESULT res = ::SendMessageTimeout (HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL);
    return res != 0;
}

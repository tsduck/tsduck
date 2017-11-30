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
#include "tsFatal.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Return the root key of a registry path.
//-----------------------------------------------------------------------------

bool ts::SplitRegistryKey(const UString& key, ::HKEY& root_key, UString& subkey)
{
    // Get end if root key name.
    const size_t sep = key.find(u'\\');

    // Root key name and subkey name.
    UString root;
    if (sep == UString::NPOS) {
        root = key;
        subkey.clear();
    }
    else {
        root = key.substr(0, sep);
        subkey = key.substr(sep + 1);
    }

    // Resolve root key handle.
    if (root.similar(u"HKEY_CLASSES_ROOT") || root.similar(u"HKCR")) {
        root_key = HKEY_CLASSES_ROOT;
    }
    else if (root.similar(u"HKEY_CURRENT_USER") || root.similar(u"HKCU")) {
        root_key = HKEY_CURRENT_USER;
    }
    else if (root.similar(u"HKEY_LOCAL_MACHINE") || root.similar(u"HKLM")) {
        root_key = HKEY_LOCAL_MACHINE;
    }
    else if (root.similar(u"HKEY_USERS") || root.similar(u"HKU")) {
        root_key = HKEY_USERS;
    }
    else if (root.similar(u"HKEY_CURRENT_CONFIG") || root.similar(u"HKCC")) {
        root_key = HKEY_CURRENT_CONFIG;
    }
    else if (root.similar(u"HKEY_PERFORMANCE_DATA") || root.similar(u"HKPD")) {
        root_key = HKEY_PERFORMANCE_DATA;
    }
    else {
        root_key = NULL;
        return false;
    }
    return true;
}

bool ts::SplitRegistryKey(const UString& key, ::HKEY& root_key, UString& midkey, UString& final_key)
{
    midkey.clear();
    const bool ok = SplitRegistryKey(key, root_key, final_key);
    if (ok) {
        const size_t sep = final_key.rfind(u'\\');
        if (sep != UString::NPOS) {
            midkey = final_key.substr(0, sep);
            final_key.erase(0, sep + 1);
        }
    }
    return ok;
}


//-----------------------------------------------------------------------------
// Get a value in a registry key as a string.
//-----------------------------------------------------------------------------

ts::UString ts::GetRegistryValue(const UString& key, const UString& value_name)
{
    // Split name
    ::HKEY root;
    UString subkey;
    if (!SplitRegistryKey(key, root, subkey)) {
        return UString();
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, subkey.wc_str(), 0, KEY_READ, &hkey);
    if (hr != ERROR_SUCCESS) {
        return UString();
    }

    // Query the the size of the value in the key. By giving a NULL address
    // to lpData, RegQueryValueEx simply returns the size of the value.
    ::DWORD type;
    ::DWORD size = 0;
    hr = ::RegQueryValueExW(hkey, value_name.wc_str(), NULL, &type, NULL, &size);
    if ((hr != ERROR_SUCCESS && hr != ERROR_MORE_DATA) || size <= 0) {
        ::RegCloseKey(hkey);
        return UString();
    }

    // Allocate new buffer and actually get the value
    ::DWORD bufsize = (size < 0 ? 0 : size) + 10;
    ::BYTE* buf = new ::BYTE[size = bufsize];
    CheckNonNull(buf);
    hr = ::RegQueryValueExW(hkey, value_name.wc_str(), NULL, &type, buf, &size);
    ::RegCloseKey(hkey);
    if (hr != ERROR_SUCCESS) {
        size = 0;
        type = REG_NONE;
    }
    size = std::max(::DWORD(0), std::min(size, bufsize - 1));
    buf[size] = 0; // if improperly terminated string

    // Convert value to a string
    UString value;
    switch (type) {
        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
            // There is at least one nul-terminated string in. If the type is REG_MULTI_SZ,
            // there are several nul-terminated strings, ending with two nuls, but we keep only the first string.
            value = reinterpret_cast<UChar*>(buf);
            break;
        case REG_DWORD:
            value = UString::Format(u"%d", {*reinterpret_cast<const ::DWORD*>(buf)});
            break;
        case REG_DWORD_BIG_ENDIAN:
            value = UString::Format(u"%d", {GetUInt32(buf)});
            break;
    }

    // Return value
    delete[] buf;
    return value;
}


//-----------------------------------------------------------------------------
// Set value of a registry key.
//-----------------------------------------------------------------------------

bool ts::SetRegistryValue(const UString& key, const UString& value_name, const UString& value, bool expandable)
{
    // Split name
    ::HKEY root;
    UString subkey;
    if (!SplitRegistryKey(key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, subkey.wc_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Set the value
    hr = ::RegSetValueExW(hkey,
                          value_name.wc_str(),
                          0, // reserved
                          expandable ? REG_EXPAND_SZ : REG_SZ,
                          reinterpret_cast<const ::BYTE*>(value.wc_str()),
                          ::DWORD(sizeof(UChar) * (value.length() + 1))); // include terminating nul

    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Set value of a registry key.
//-----------------------------------------------------------------------------

bool ts::SetRegistryValue(const UString& key, const UString& value_name, ::DWORD value)
{
    // Split name
    ::HKEY root;
    UString subkey;
    if (!SplitRegistryKey(key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, subkey.wc_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Set the value
    hr = ::RegSetValueExW(hkey,
                          value_name.wc_str(),
                          0, // reserved
                          REG_DWORD,
                          reinterpret_cast<const ::BYTE*> (&value),
                          sizeof(value));

    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a value of a registry key.
//-----------------------------------------------------------------------------

bool ts::DeleteRegistryValue(const UString& key, const UString& value_name)
{
    // Split name
    ::HKEY root;
    UString subkey;
    if (!SplitRegistryKey(key, root, subkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, subkey.wc_str(), 0, KEY_SET_VALUE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Delete the value
    hr = ::RegDeleteValueW(hkey, value_name.wc_str());
    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Create a registry key.
//-----------------------------------------------------------------------------

bool ts::CreateRegistryKey(const UString& key, bool is_volatile)
{
    // Split name
    ::HKEY root;
    UString midkey, newkey;
    if (!SplitRegistryKey(key, root, midkey, newkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, midkey.wc_str(), 0, KEY_CREATE_SUB_KEY | KEY_READ, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Create the key
    ::HKEY hnewkey;
    hr = ::RegCreateKeyExW(hkey,
                           newkey.wc_str(),
                           0,  // reserved
                           NULL, // class
                           is_volatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                           0, // security: no further access
                           NULL, // security attributes
                           &hnewkey,
                           NULL); // disposition

    bool success = hr == ERROR_SUCCESS;
    if (success) {
        ::RegCloseKey(hnewkey);
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a registry key.
//-----------------------------------------------------------------------------

bool ts::DeleteRegistryKey(const UString& key)
{
    // Split name
    ::HKEY root;
    UString midkey, newkey;
    if (!SplitRegistryKey(key, root, midkey, newkey)) {
        return false;
    }

    // Open registry key
    ::HKEY hkey;
    ::LONG hr = ::RegOpenKeyExW(root, midkey.wc_str(), 0, KEY_WRITE, &hkey);
    if (hr != ERROR_SUCCESS) {
        return false;
    }

    // Delete the key
    hr = ::RegDeleteKeyW(hkey, newkey.wc_str());
    bool success = hr == ERROR_SUCCESS;
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Notify all applications of a setting change.
//-----------------------------------------------------------------------------

bool ts::NotifySettingChange()
{
    // timeout: 5000 ms
    ::LRESULT res = ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL);
    return res != 0;
}

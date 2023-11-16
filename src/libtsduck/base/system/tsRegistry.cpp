//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsRegistry.h"
#include "tsMemory.h"
#include "tsSysUtils.h"
#include "tsFatal.h"


//-----------------------------------------------------------------------------
// Error report on non-Windows systems
//-----------------------------------------------------------------------------

#if !defined(TS_WINDOWS)

#define ERROR_MESSAGE  u"not Windows, no registry"
#define BOOL_ERROR     { report.error(ERROR_MESSAGE); return false; }
#define USTRING_ERROR  { report.error(ERROR_MESSAGE); return UString(); }

const ts::UString ts::Registry::SystemEnvironmentKey;
const ts::UString ts::Registry::UserEnvironmentKey;

bool ts::Registry::SplitKey(const UString&, Handle&, UString&, Report& report) BOOL_ERROR
bool ts::Registry::SplitKey(const UString&, Handle&, UString&, UString&, Report& report) BOOL_ERROR
ts::UString ts::Registry::GetValue(const UString&, const UString&, Report& report) USTRING_ERROR
bool ts::Registry::SetValue(const UString&, const UString&, const UString&, bool, Report& report) BOOL_ERROR
bool ts::Registry::SetValue(const UString&, const UString&, uint32_t, Report& report) BOOL_ERROR
bool ts::Registry::DeleteValue(const UString&, const UString&, Report& report) BOOL_ERROR
bool ts::Registry::CreateKey(const UString&, bool, Report& report) BOOL_ERROR
bool ts::Registry::DeleteKey(const UString&, Report& report) BOOL_ERROR
bool ts::Registry::NotifySettingChange(Report& report) BOOL_ERROR
bool ts::Registry::NotifyEnvironmentChange(Report& report) BOOL_ERROR

#else

//-----------------------------------------------------------------------------
// Windows implementation.
//-----------------------------------------------------------------------------

// Name of the registry key containining the system-defined environment variables.
const ts::UString ts::Registry::SystemEnvironmentKey(u"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
const ts::UString ts::Registry::UserEnvironmentKey(u"HKCU\\Environment");


//-----------------------------------------------------------------------------
// Return the root key of a registry path.
//-----------------------------------------------------------------------------

bool ts::Registry::SplitKey(const UString& key, Handle& root_key, UString& subkey, Report& report)
{
    // Get end if root key name.
    const size_t sep = key.find(u'\\');

    // Root key name and subkey name.
    UString root;
    if (sep == NPOS) {
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
        report.error(u"invalid root key \"%s\"", {root});
        root_key = nullptr;
        return false;
    }
    return true;
}

bool ts::Registry::SplitKey(const UString& key, Handle& root_key, UString& midkey, UString& final_key, Report& report)
{
    midkey.clear();
    const bool ok = SplitKey(key, root_key, final_key, report);
    if (ok) {
        const size_t sep = final_key.rfind(u'\\');
        if (sep != NPOS) {
            midkey = final_key.substr(0, sep);
            final_key.erase(0, sep + 1);
        }
    }
    return ok;
}


//-----------------------------------------------------------------------------
// Open a registry key.
//-----------------------------------------------------------------------------

bool ts::Registry::OpenKey(Handle root, const UString& key, ::REGSAM sam, Handle& handle, Report& report)
{
    ::LONG hr = ::RegOpenKeyExW(root, key.wc_str(), 0, sam, &handle);
    if (hr == ERROR_SUCCESS) {
        return true;
    }
    else {
        report.error(u"error opening key %s: %s", {key, SysErrorCodeMessage(hr)});
        return false;
    }
}


//-----------------------------------------------------------------------------
// Get a value in a registry key as a string.
//-----------------------------------------------------------------------------

ts::UString ts::Registry::GetValue(const UString& key, const UString& value_name, Report& report)
{
    // Split name
    Handle root, hkey;
    UString subkey;
    if (!SplitKey(key, root, subkey, report) || !OpenKey(root, subkey, KEY_READ, hkey, report)) {
        return UString();
    }

    // Query the the size of the value in the key. By giving a NULL address
    // to lpData, RegQueryValueEx simply returns the size of the value.
    ::DWORD type;
    ::DWORD size = 0;
    ::LONG hr = ::RegQueryValueExW(hkey, value_name.wc_str(), nullptr, &type, nullptr, &size);
    if ((hr != ERROR_SUCCESS && hr != ERROR_MORE_DATA) || size <= 0) {
        report.error(u"error querying %s\\%s: %s", {key, value_name, SysErrorCodeMessage(hr)});
        ::RegCloseKey(hkey);
        return UString();
    }

    // Allocate new buffer and actually get the value
    ::DWORD bufsize = size + 10;
    ::BYTE* buf = new ::BYTE[size = bufsize];
    CheckNonNull(buf);
    hr = ::RegQueryValueExW(hkey, value_name.wc_str(), nullptr, &type, buf, &size);
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

bool ts::Registry::SetValue(const UString& key, const UString& value_name, const UString& value, bool expandable, Report& report)
{
    // Split name
    ::HKEY root, hkey;
    UString subkey;
    if (!SplitKey(key, root, subkey, report) || !OpenKey(root, subkey, KEY_WRITE, hkey, report)) {
        return false;
    }

    // Set the value
    ::LONG hr = ::RegSetValueExW(hkey,
                                 value_name.wc_str(),
                                 0, // reserved
                                 expandable ? REG_EXPAND_SZ : REG_SZ,
                                 reinterpret_cast<const ::BYTE*>(value.wc_str()),
                                 ::DWORD(sizeof(UChar) * (value.length() + 1))); // include terminating nul

    const bool success = hr == ERROR_SUCCESS;
    if (!success) {
        report.error(u"error setting %s\\%s: %s", {key, value_name, SysErrorCodeMessage(hr)});
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Set value of a registry key.
//-----------------------------------------------------------------------------

bool ts::Registry::SetValue(const UString& key, const UString& value_name, uint32_t value, Report& report)
{
    // Split name
    ::HKEY root, hkey;
    UString subkey;
    if (!SplitKey(key, root, subkey, report) || !OpenKey(root, subkey, KEY_WRITE, hkey, report)) {
        return false;
    }

    // Set the value
    ::LONG hr = ::RegSetValueExW(hkey,
                                 value_name.wc_str(),
                                 0, // reserved
                                 REG_DWORD,
                                 reinterpret_cast<const ::BYTE*> (&value),
                                 sizeof(value));

    const bool success = hr == ERROR_SUCCESS;
    if (!success) {
        report.error(u"error setting %s\\%s: %s", {key, value_name, SysErrorCodeMessage(hr)});
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a value of a registry key.
//-----------------------------------------------------------------------------

bool ts::Registry::DeleteValue(const UString& key, const UString& value_name, Report& report)
{
    // Split name
    ::HKEY root, hkey;
    UString subkey;
    if (!SplitKey(key, root, subkey, report) || !OpenKey(root, subkey, KEY_SET_VALUE, hkey, report)) {
        return false;
    }

    // Delete the value
    const ::LONG hr = ::RegDeleteValueW(hkey, value_name.wc_str());
    const bool success = hr == ERROR_SUCCESS;
    if (!success) {
        report.error(u"error deleting %s\\%s: %s", {key, value_name, SysErrorCodeMessage(hr)});
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Create a registry key.
//-----------------------------------------------------------------------------

bool ts::Registry::CreateKey(const UString& key, bool is_volatile, Report& report)
{
    // Split name
    ::HKEY root, hkey;
    UString midkey, newkey;
    if (!SplitKey(key, root, midkey, newkey, report) || !OpenKey(root, midkey, KEY_CREATE_SUB_KEY | KEY_READ, hkey, report)) {
        return false;
    }

    // Create the key
    ::HKEY hnewkey;
    const ::LONG hr = ::RegCreateKeyExW(hkey,
                                        newkey.wc_str(),
                                        0,  // reserved
                                        nullptr, // class
                                        is_volatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                                        0, // security: no further access
                                        nullptr, // security attributes
                                        &hnewkey,
                                        nullptr); // disposition

    const bool success = hr == ERROR_SUCCESS;
    if (success) {
        ::RegCloseKey(hnewkey);
    }
    else {
        report.error(u"error creating %s: %s", {key, SysErrorCodeMessage(hr)});
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Delete a registry key.
//-----------------------------------------------------------------------------

bool ts::Registry::DeleteKey(const UString& key, Report& report)
{
    // Split name
    ::HKEY root, hkey;
    UString midkey, newkey;
    if (!SplitKey(key, root, midkey, newkey, report) || !OpenKey(root, midkey, KEY_WRITE, hkey, report)) {
        return false;
    }

    // Delete the key
    const ::LONG hr = ::RegDeleteKeyW(hkey, newkey.wc_str());
    const bool success = hr == ERROR_SUCCESS;
    if (!success) {
        report.error(u"error deleting %s: %s", {key, SysErrorCodeMessage(hr)});
    }
    ::RegCloseKey(hkey);
    return success;
}


//-----------------------------------------------------------------------------
// Notify all applications of a setting change.
//-----------------------------------------------------------------------------

bool ts::Registry::NotifySettingChangeParam(const void* param, uint32_t timeout_ms, Report& report)
{
    const bool ok = 0 != ::SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(param), SMTO_ABORTIFHUNG, ::UINT(timeout_ms), 0);
    if (!ok) {
        report.error(u"notification error: %s", {SysErrorCodeMessage()});
    }
    return ok;
}

bool ts::Registry::NotifySettingChange(Report& report)
{
    return NotifySettingChangeParam(0, 5000, report);
}

bool ts::Registry::NotifyEnvironmentChange(Report& report)
{
    // Warning: We call SendMessageTimeoutW (wide version, with "W").
    // So, the parameter must be a wide string L"Environment".
    // If we call SendMessageTimeoutA, we need an ASCII string "Environment".
    return NotifySettingChangeParam(L"Environment", 5000, report);
}

#endif // TS_WINDOWS

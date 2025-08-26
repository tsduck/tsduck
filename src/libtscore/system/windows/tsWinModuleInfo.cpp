//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWinModuleInfo.h"
#include "tsWinUtils.h"
#include "tsByteBlock.h"

// Required link libraries.
#if defined(TS_MSC)
    #pragma comment(lib, "version.lib")
#endif


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::WinModuleInfo::WinModuleInfo(const UString& file_name)
{
    reload(file_name);
}

ts::WinModuleInfo::WinModuleInfo(const void* address, std::nullptr_t)
{
    reloadByAddress(address);
}


//----------------------------------------------------------------------------
// Clear the content of the structure.
//----------------------------------------------------------------------------

void ts::WinModuleInfo::clear()
{
    file_version_int = 0;
    product_version_int = 0;
    _last_error.clear();
    for (const auto& it : GetNames()) {
        (this->*(it.first)).clear();
    }
}


//----------------------------------------------------------------------------
// Get a list of Windows-defined names and their corresponding string field.
//----------------------------------------------------------------------------

const std::vector<std::pair<ts::UString ts::WinModuleInfo::*, ts::UString>>& ts::WinModuleInfo::GetNames()
{
    static const std::vector<std::pair<ts::UString ts::WinModuleInfo::*, ts::UString>> data {
        {&WinModuleInfo::comments,           u"Comments"},
        {&WinModuleInfo::company_name,       u"CompanyName"},
        {&WinModuleInfo::file_description,   u"FileDescription"},
        {&WinModuleInfo::file_version,       u"FileVersion"},
        {&WinModuleInfo::internal_name,      u"InternalName"},
        {&WinModuleInfo::legal_copyright,    u"LegalCopyright"},
        {&WinModuleInfo::legal_trademarks,   u"LegalTrademarks"},
        {&WinModuleInfo::original_file_name, u"OriginalFilename"},
        {&WinModuleInfo::product_name,       u"ProductName"},
        {&WinModuleInfo::product_version,    u"ProductVersion"},
        {&WinModuleInfo::private_build,      u"PrivateBuild"},
        {&WinModuleInfo::special_build,      u"SpecialBuild"},
    };
    return data;
}


//----------------------------------------------------------------------------
// Reload content from a file name.
//----------------------------------------------------------------------------

bool ts::WinModuleInfo::reload(const UString& file_name)
{
    clear();

    // Get the blob of version information for the module file.
    ByteBlock blob;
    const ::DWORD blob_size = ::GetFileVersionInfoSizeW(file_name.wc_str(), nullptr);
    bool success = blob_size > 0;
    ::DWORD err = ::GetLastError();
    if (success) {
        blob.resize(size_t(blob_size));
        success = ::GetFileVersionInfoW(file_name.wc_str(), 0, blob_size, blob.data());
    }
    if (!success) {
        _last_error.format(u"error getting version info for %s: %s", file_name, WinErrorMessage(err));
        return false;
    }

    // Locate the FIXEDFILEINFO inside the blob.
    ::VS_FIXEDFILEINFO* info = nullptr;
    ::UINT len = 0;
    if (!::VerQueryValueW(blob.data(), L"\\", reinterpret_cast<::LPVOID*>(&info), &len) || info == nullptr || len < sizeof(*info)) {
        _last_error.format(u"no FIXEDFILEINFO found for %s", file_name);
        return false;
    }

    // Build versions in integer format.
    file_version_int = (uint64_t(info->dwFileVersionMS) << 32) + info->dwFileVersionLS;
    product_version_int = (uint64_t(info->dwProductVersionMS) << 32) + info->dwProductVersionLS;

    // Get translation table for the module.
    struct LanguageCode {
        ::WORD language = 0;
        ::WORD code_page = 0;
    };
    LanguageCode* trans = nullptr;
    ::UINT trans_count = 0;
    success = ::VerQueryValueW(blob.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<LPVOID*>(&trans), &trans_count);
    trans_count /= sizeof(LanguageCode);
    if (!success || trans == nullptr || trans_count == 0) {
        _last_error.format(u"no translation found for %s", file_name);
        return false;
    }

    // Locate US-English language code.
    size_t en_index = 0;
    while (en_index < trans_count && trans[en_index].language != US_ENGLISH_CODE) {
        en_index++;
    }
    if (en_index >= trans_count) {
        // US-English not found, use first code by default.
        en_index = 0;
    }

    // Load all standard strings.
    for (const auto& it : GetNames()) {
        UString sub;
        sub.format(u"\\StringFileInfo\\%04x%04x\\%s", trans[en_index].language, trans[en_index].code_page, it.second);
        UChar* value = nullptr;
        if (::VerQueryValueW(blob.data(), sub.wc_str(), reinterpret_cast<::LPVOID*>(&value), &len) && value != nullptr && len > 0) {
            // Get actual length of the string.
            for (size_t i = 0; i < len; ++i) {
                if (value[i] == 0) {
                    len = ::UINT(i);
                }
            }
            this->*(it.first) = UString(value, len);
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Reload content from an address in memory.
//----------------------------------------------------------------------------

bool ts::WinModuleInfo::reloadByAddress(const void* address)
{
    clear();

    // Get a handle to the module containing the address.
    // Do not increment the handle reference counter. The module is already loaded
    // by definition, the handle is already valid, and we won't unload it here.
    ::HMODULE mod_handle = nullptr;
    static constexpr ::DWORD mod_flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (!::GetModuleHandleExW(mod_flags, reinterpret_cast<::LPCWSTR>(address), &mod_handle)) {
        // No module found for that address.
        const ::DWORD err = ::GetLastError();
        _last_error.format(u"address 0x%X not found in any module: %s", uintptr_t(address), WinErrorMessage(err));
        return false;
    }

    // Get the module file name. Loop until the buffer is large enough.
    std::vector<::WCHAR> mod_path(1024);
    for (int guard = 5; guard > 0; --guard) {
        const ::DWORD len = ::GetModuleFileNameW(mod_handle, mod_path.data(), ::DWORD(mod_path.size()));
        const ::DWORD err = ::GetLastError();
        if (len <= 0) {
            // Failure.
            _last_error.format(u"error getting module name for address 0x%X: %s", uintptr_t(address), WinErrorMessage(err));
            return false;
        }
        else if (len == ::DWORD(mod_path.size()) && err == ERROR_INSUFFICIENT_BUFFER) {
            // Output buffer was too small.
            mod_path.resize(2 * mod_path.size());
        }
        else {
            // Full path returned.
            break;
        }
    }

    // Load using module file name.
    return reload(mod_path.data());
}


//----------------------------------------------------------------------------
// Get a summary string describing the module.
//----------------------------------------------------------------------------

ts::UString ts::WinModuleInfo::summary() const
{
    // Return the error message if invalid.
    if (!_last_error.empty()) {
        return _last_error;
    }

    // Build various parts.
    UStringList parts;
    if (!file_description.empty()) {
        parts.push_back(file_description);
    }
    UString name(original_file_name);
    if (name.ends_with(u".mui", CASE_INSENSITIVE)) {
        name.resize(name.size() - 4);
    }
    if (name.empty()) {
        name = internal_name;
    }
    if (!name.empty()) {
        parts.push_back(name);
    }
    if (!file_version.empty()) {
        parts.push_back(u"version " + file_version);
    }
    else {
        parts.push_back(UString::Format(u"version %d.%d.%d.%d",
                                        file_version_int >> 48, (file_version_int >> 32) & 0xFFFF, (file_version_int >> 16) & 0xFFFF, file_version_int & 0xFFFF));
    }
    return UString::Join(parts, u", ");
}

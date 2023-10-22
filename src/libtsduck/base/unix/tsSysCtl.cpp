//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSysCtl.h"
#include <ctype.h>


//----------------------------------------------------------------------------
// Get a Unix sysctl(2) boolean value by name.
//----------------------------------------------------------------------------

bool ts::SysCtrlBool(const std::string& name)
{
#if defined(TS_MAC)
    int val = 0;
    size_t len = sizeof(val);
    return ::sysctlbyname(name.c_str(), &val, &len, nullptr, 0) == 0 && val != 0;
#else
    return false;
#endif
}


//----------------------------------------------------------------------------
// Get a Unix sysctl(2) string value.
//----------------------------------------------------------------------------

ts::UString ts::SysCtrlString(std::initializer_list<int> oid)
{
#if defined(TS_MAC) || defined(TS_BSD)

    std::vector<int> vecoid(oid.begin(), oid.end());

    // First step, get the returned size of the string.
    size_t length = 0;
    if (::sysctl(&vecoid[0], u_int(vecoid.size()), nullptr, &length, nullptr, 0) < 0) {
        return UString();
    }

    // Then get the string with the right buffer size.
    std::string name(length, '\0');
    if (::sysctl(&vecoid[0], u_int(vecoid.size()), &name[0], &length, nullptr, 0) < 0) {
        return UString();
    }

    // Cleanup end of string.
    while (!name.empty() && (name.back() == '\0' || ::isspace(name.back()))) {
        name.pop_back();
    }

    return UString::FromUTF8(name);

#else

    // sysctl(2) not implemented on this platform.
    return UString();

#endif
}


//----------------------------------------------------------------------------
// Get a Unix sysctl(2) binary value.
//----------------------------------------------------------------------------

ts::ByteBlock ts::SysCtrlBytes(std::initializer_list<int> oid)
{
#if defined(TS_MAC) || defined(TS_BSD)

    std::vector<int> vecoid(oid.begin(), oid.end());

    // First step, get the returned size of the value.
    size_t length = 0;
    if (::sysctl(&vecoid[0], u_int(vecoid.size()), nullptr, &length, nullptr, 0) < 0) {
        return ByteBlock();
    }

    // Then get the value with the right buffer size.
    ByteBlock value(length, 0);
    if (::sysctl(&vecoid[0], u_int(vecoid.size()), value.data(), &length, nullptr, 0) < 0) {
        return ByteBlock();
    }

    return value;

#else

    // sysctl(2) not implemented on this platform.
    return ByteBlock();

#endif
}

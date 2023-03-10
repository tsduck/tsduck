//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

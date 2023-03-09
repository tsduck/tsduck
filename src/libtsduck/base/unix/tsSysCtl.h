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
//!
//!  @file
//!  @ingroup unix
//!  Reading Unix sysctl(2) values.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsUString.h"

// Definition of sysctl OID codes.
#if defined(TS_MAC) || defined(TS_BSD)
    #include "tsBeforeStandardHeaders.h"
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Get a Unix sysctl(2) boolean value by name.
    //! This function now works on BSD systems only (macOS, FreeBSD, OpenBSD, DragonFlyBSD).
    //! Linux no longer supports sysctl(2), replaced by the /proc/sys filesystem.
    //! @param [in] name Name of the data to return.
    //! @return The bool value. False if not found.
    //!
    TSDUCKDLL bool SysCtrlBool(const std::string& name);
    //!
    //! Get a Unix sysctl(2) string value.
    //! This function now works on BSD systems only (macOS, FreeBSD, OpenBSD, DragonFlyBSD).
    //! Linux no longer supports sysctl(2), replaced by the /proc/sys filesystem.
    //! @param [in] oid Identifier of the data to return as a list of int values.
    //! @return The string value or empty if not found.
    //!
    TSDUCKDLL UString SysCtrlString(std::initializer_list<int> oid);
    //!
    //! Get a Unix sysctl(2) binary value.
    //! This function now works on BSD systems only (macOS, FreeBSD, OpenBSD, DragonFlyBSD).
    //! Linux no longer supports sysctl(2), replaced by the /proc/sys filesystem.
    //! @param [in] oid Identifier of the data to return as a list of int values.
    //! @return The binary value or empty if not found.
    //!
    TSDUCKDLL ByteBlock SysCtrlBytes(std::initializer_list<int> oid);
}

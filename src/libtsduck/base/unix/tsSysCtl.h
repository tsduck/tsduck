//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

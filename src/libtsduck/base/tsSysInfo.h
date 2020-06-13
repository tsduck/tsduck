//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  A singleton holding information on the current operating system.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingletonManager.h"
#include "tsUString.h"

namespace ts {
    //!
    //! A singleton holding information on the current operating system.
    //! @ingroup system
    //!
    class TSDUCKDLL SysInfo
    {
        TS_DECLARE_SINGLETON(SysInfo);

    public:
        //!
        //! Check if the running operating system is Linux.
        //! @return True if the running operating system is Linux.
        //!
        bool isLinux() const { return _isLinux; }
        //!
        //! Check if the running operating system is Fedora Linux.
        //! @return True if the running operating system is Fedora Linux.
        //!
        bool isFedora() const { return _isFedora; }
        //!
        //! Check if the running operating system is RedHat Entreprise Linux or one of its clone such as CentOS.
        //! @return True if the running operating system is RedHat.
        //!
        bool isRedHat() const { return _isRedHat; }
        //!
        //! Check if the running operating system is Linux Ubuntu.
        //! @return True if the running operating system is Linux Ubuntu.
        //!
        bool isUbuntu() const { return _isUbuntu; }
        //!
        //! Check if the running operating system is Linux Debian.
        //! Note that Ubuntu is not considered a real Debian.
        //! @return True if the running operating system is Linux Debian.
        //!
        bool isDebian() const { return _isDebian; }
        //!
        //! Check if the running operating system is macOS.
        //! @return True if the running operating system is macOS.
        //!
        bool isMacOS() const { return _isMacOS; }
        //!
        //! Check if the running operating system is Windows.
        //! @return True if the running operating system is Windows.
        //!
        bool isWindows() const { return _isWindows; }
        //!
        //! Check if the CPU is Intel IA-32, also known as x86.
        //! @return True if the CPU is Intel IA-32.
        //!
        bool isIntel32() const { return _isIntel32; }
        //!
        //! Check if the CPU is the 64-bit extension of the IA-32 architecture, also known as AMD-64 or Intel x86-64.
        //! @return True if the CPU is the 64-bit extension of IA-32.
        //!
        bool isIntel64() const { return _isIntel64; }
        //!
        //! Get the operating system version.
        //! @return The operating system version.
        //!
        UString systemVersion() const { return _systemVersion; }
        //!
        //! Get the operating system name.
        //! @return The operating system name.
        //!
        UString systemName() const { return _systemName; }
        //!
        //! Get the name of the system host.
        //! @return The name of the system host.
        //!
        UString hostName() const { return _hostName; }
        //!
        //! Get system memory page size.
        //! @return The system memory page size in bytes.
        //!
        size_t memoryPageSize() const { return _memoryPageSize; }

    private:
        bool    _isLinux;
        bool    _isFedora;
        bool    _isRedHat;
        bool    _isUbuntu;
        bool    _isDebian;
        bool    _isMacOS;
        bool    _isWindows;
        bool    _isIntel32;
        bool    _isIntel64;
        UString _systemVersion;
        UString _systemName;
        UString _hostName;
        size_t  _memoryPageSize;
    };
}

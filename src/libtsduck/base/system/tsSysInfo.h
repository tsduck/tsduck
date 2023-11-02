//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton holding information on the current operating system.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"
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
        //! Check if the running operating system is RedHat Entreprise Linux or one of its clone such as AlmaLinux.
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
        //! Check if the running operating system is Linux Raspbian (Debian derivative on Raspberry Pi).
        //! @return True if the running operating system is Linux Debian.
        //!
        bool isRaspbian() const { return _isRaspbian; }
        //!
        //! Check if the running operating system is macOS.
        //! @return True if the running operating system is macOS.
        //!
        bool isMacOS() const { return _isMacOS; }
        //!
        //! Check if the running operating system is a BSD system (FreeBSD, NetBSD, OpenBSD, DragonFlyBSD).
        //! @return True if the running operating system is FreeBSD.
        //!
        bool isBSD() const { return _isBSD; }
        //!
        //! Check if the running operating system is FreeBSD.
        //! @return True if the running operating system is FreeBSD.
        //!
        bool isFreeBSD() const { return _isFreeBSD; }
        //!
        //! Check if the running operating system is NetBSD.
        //! @return True if the running operating system is NetBSD.
        //!
        bool isNetBSD() const { return _isNetBSD; }
        //!
        //! Check if the running operating system is OpenBSD.
        //! @return True if the running operating system is OpenBSD.
        //!
        bool isOpenBSD() const { return _isOpenBSD; }
        //!
        //! Check if the running operating system is DragonFlyBSD.
        //! @return True if the running operating system is DragonFlyBSD.
        //!
        bool isDragonFlyBSD() const { return _isDragonFlyBSD; }
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
        //! Check if the CPU is Arm-32.
        //! @return True if the CPU is Arm-32.
        //!
        bool isArm32() const { return _isArm32; }
        //!
        //! Check if the CPU is Arm-64, also known as aarch64.
        //! @return True if the CPU is Arm-64.
        //!
        bool isArm64() const { return _isArm64; }
        //!
        //! Check if the CPU supports accelerated instructions for CRC32 computation.
        //! @return True if the CPU supports CRC32 instructions.
        //!
        bool crcInstructions() const { return _crcInstructions; }
        //!
        //! Check if the CPU supports accelerated instructions for AES.
        //! @return True if the CPU supports AES instructions.
        //!
        bool aesInstructions() const { return _aesInstructions; }
        //!
        //! Check if the CPU supports accelerated instructions for SHA-1.
        //! @return True if the CPU supports SHA-1 instructions.
        //!
        bool sha1Instructions() const { return _sha1Instructions; }
        //!
        //! Check if the CPU supports accelerated instructions for SHA-256.
        //! @return True if the CPU supports SHA-256 instructions.
        //!
        bool sha256Instructions() const { return _sha256Instructions; }
        //!
        //! Check if the CPU supports accelerated instructions for SHA-512.
        //! @return True if the CPU supports SHA-512 instructions.
        //!
        bool sha512Instructions() const { return _sha512Instructions; }
        //!
        //! Get the operating system version.
        //! @return The operating system version.
        //!
        UString systemVersion() const { return _systemVersion; }
        //!
        //! Get the operating system major version as an integer.
        //! The exact meaning of this number is system dependent.
        //! @return The operating system major version or -1 if unknown.
        //!
        int systemMajorVersion() const { return _systemMajorVersion; }
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
        //! Get the name of the CPU architecure.
        //! @return The name of the CPU architecure.
        //!
        UString cpuName() const { return _cpuName; }
        //!
        //! Get system memory page size.
        //! @return The system memory page size in bytes.
        //!
        size_t memoryPageSize() const { return _memoryPageSize; }

    private:
        bool    _isLinux = false;
        bool    _isFedora = false;
        bool    _isRedHat = false;
        bool    _isUbuntu = false;
        bool    _isDebian = false;
        bool    _isRaspbian = false;
        bool    _isMacOS = false;
        bool    _isBSD = false;
        bool    _isFreeBSD = false;
        bool    _isNetBSD = false;
        bool    _isOpenBSD = false;
        bool    _isDragonFlyBSD = false;
        bool    _isWindows = false;
        bool    _isIntel32 = false;
        bool    _isIntel64 = false;
        bool    _isArm32 = false;
        bool    _isArm64 = false;
        bool    _crcInstructions = false;
        bool    _aesInstructions = false;
        bool    _sha1Instructions = false;
        bool    _sha256Instructions = false;
        bool    _sha512Instructions = false;
        int     _systemMajorVersion {-1};
        UString _systemVersion {};
        UString _systemName {};
        UString _hostName {};
        UString _cpuName {};
        size_t  _memoryPageSize = 0;
    };
}

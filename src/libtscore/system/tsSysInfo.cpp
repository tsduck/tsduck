//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSysInfo.h"
#include "tsEnvironment.h"
#include "tsMemory.h"
#include "tsCryptoAcceleration.h"
#include "tsFeatures.h"

#if defined(TS_LINUX)
    #include <sys/auxv.h>
#endif

#if defined(TS_MAC)
    #include "tsMacPList.h"
#endif

#if defined(TS_UNIX)
    #include "tsSysCtl.h"
#endif

TS_DEFINE_SINGLETON(ts::SysInfo);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::SysInfo::SysInfo() :

#if defined(TS_I386)
    _arch(INTEL32),
    _cpuName(u"Intel x86"),
#elif defined(TS_X86_64)
    _arch(INTEL64),
    _cpuName(u"Intel x86-64"),
#elif defined(TS_ARM64)
    _arch(ARM64),
    _cpuName(u"Arm-64"),
#elif defined(TS_ARM32)
    _arch(ARM32),
    _cpuName(u"Arm-32"),
#elif defined(TS_MIPS64)
    _arch(MIPS64),
    _cpuName(u"MIPS64"),
#elif defined(TS_MIPS)
    _arch(MIPS32),
    _cpuName(u"MIPS"),
#elif defined(TS_SPARC)
    _arch(SPARC),
    _cpuName(u"SPARC"),
#elif defined(TS_POWERPC64)
    _arch(PPC64),
    _cpuName(u"PowerPC-64"),
#elif defined(TS_POWERPC)
    _arch(PPC32),
    _cpuName(u"PowerPC"),
#elif defined(TS_RISCV64)
    _arch(RISCV64),
    _cpuName(u"RISCV-64"),
#elif defined(TS_S390X)
    _arch(S390X),
    _cpuName(u"S390X"),
#else
    #error "Unsupported CPU architecture"
#endif

#if defined(TS_LINUX)
    _osFamily(LINUX),
    _osFlavor(UNKNOWN),
    _systemName(u"Linux"),
#elif defined(TS_MAC)
    _osFamily(MACOS),
    _osFlavor(NONE),
    _systemName(u"macOS"),
#elif defined(TS_FREEBSD)
    _osFamily(BSD),
    _osFlavor(FREEBSD),
    _systemName(u"FreeBSD"),
#elif defined(TS_NETBSD)
    _osFamily(BSD),
    _osFlavor(NETBSD),
    _systemName(u"NetBSD"),
#elif defined(TS_OPENBSD)
    _osFamily(BSD),
    _osFlavor(OPENBSD),
    _systemName(u"OpenBSD"),
#elif defined(TS_DRAGONFLYBSD)
    _osFamily(BSD),
    _osFlavor(DFLYBSD),
    _systemName(u"DragonFlyBSD"),
#elif defined(TS_WINDOWS)
    _osFamily(WINDOWS),
    _osFlavor(NONE),
    _systemName(u"Windows"),
#else
    #error "Unsupported operating system"
#endif

    _memoryPageSize(0)
{
    //
    // Get operating system name and version.
    //
#if defined(TS_LINUX)

    // On Linux, the actual system shall be determined dynamically.
    UStringList lines;
    Environment env;
    if (UString::Load(lines, u"/etc/fedora-release") && !lines.empty()) {
        _osFlavor = FEDORA;
        _systemName = u"Fedora";
        _systemVersion = lines.front();
    }
    else if (UString::Load(lines, u"/etc/redhat-release") && !lines.empty()) {
        _osFlavor = REDHAT;
        _systemName = u"Red Hat Entreprise Linux";
        _systemVersion = lines.front();
    }
    else if (UString::Load(lines, u"/etc/alpine-release") && !lines.empty()) {
        _osFlavor = ALPINE;
        _systemName = u"Alpine Linux";
        _systemVersion = lines.front();
    }
    else if (LoadEnvironment(env, u"/etc/lsb-release")) {
        if (!env[u"DISTRIB_ID"].empty()) {
            _systemName = env[u"DISTRIB_ID"];
        }
        _systemVersion = env[u"DISTRIB_DESCRIPTION"];
        if (_systemVersion.empty()) {
            _systemVersion = env[u"DISTRIB_RELEASE"];
        }
        if (_systemName.similar(u"Ubuntu")) {
            _osFlavor = UBUNTU;
        }
        else if (_systemName.similar(u"Debian")) {
            _osFlavor = DEBIAN;
        }
        else if (_systemName.similar(u"Raspbian")) {
            _osFlavor = RASPBIAN;
        }
    }
    if (_systemName == "Linux" && UString::Load(lines, u"/etc/debian_version") && !lines.empty()) {
        _systemName = u"Debian";
        if (_osFlavor == UNKNOWN) {
            _osFlavor = DEBIAN;
        }
        if (_systemVersion.empty()) {
            _systemVersion = u"Debian " + lines.front();
        }
    }

#elif defined(TS_MAC)

    // Get system version.
    MacPList sysList(u"/System/Library/CoreServices/SystemVersion.plist");
    const UString sysName(sysList[u"ProductName"]);
    const UString sysVersion(sysList[u"ProductVersion"]);
    if (!sysName.empty() && !sysVersion.empty()) {
        _systemName = sysName;
        _systemVersion = sysName + u" " + sysVersion;
    }

    // Get kernel version.
    UString osrelease(SysCtrlString({CTL_KERN, KERN_OSRELEASE}));
    osrelease.trim();
    if (!osrelease.empty()) {
        if (!_systemVersion.empty()) {
            _systemVersion += u", ";
        }
        _systemVersion += u"Darwin " + osrelease;
    }

#elif defined(TS_BSD)

    const UString sysname(SysCtrlString({CTL_KERN, KERN_OSTYPE}));
    if (!sysname.empty()) {
        _systemName = sysname;
    }

    UString osrelease(SysCtrlString({CTL_KERN, KERN_OSRELEASE}));
    osrelease.trim();
    if (osrelease.empty()) {
        _systemVersion = SysCtrlString({CTL_KERN, KERN_VERSION});
        _systemVersion.trim();
        // BSD systems tend to have long multi-line descriptions, keep only the first line.
        const size_t eol = _systemVersion.find(u'\n');
        if (eol != NPOS) {
            _systemVersion.resize(eol);
            _systemVersion.trim(true, true, true);
        }
    }
    else if (_systemName.empty()) {
        _systemVersion = osrelease;
    }
    else {
        _systemVersion = _systemName + u" " + osrelease;
    }

#elif defined(TS_WINDOWS)

    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(deprecated-declarations)
    TS_LLVM_NOWARNING(deprecated-declarations)
    TS_MSC_NOWARNING(4996) // warning C4996: 'GetVersionExW': was declared deprecated

    // System version.
    ::OSVERSIONINFOW info;
    TS_ZERO(info);
    info.dwOSVersionInfoSize = sizeof(info);
    if (::GetVersionExW(&info)) {
        _systemVersion = UString::Format(u"Windows %d.%d Build %d %s", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber, UString(info.szCSDVersion));
        _systemVersion.trim();
    }

    TS_POP_WARNING()

    // Detect 32-bit application on 64-bit system.
    ::BOOL wow64 = 0;
    if (::IsWow64Process(::GetCurrentProcess(), &wow64) && wow64) {
        // 32-bit application on 64-bit system => set system characteristics, not application.
        _arch = INTEL64;
        _cpuName = u"Intel x86-64";
    }

#endif

    // System version defaults to system name.
    if (_systemVersion.empty()) {
        _systemVersion = _systemName;
    }

    // System major version defaults to the first integer field in the system version string.
    if (_systemMajorVersion < 0) {
        const size_t start = _systemVersion.find_first_of(u"0123456789");
        if (start != NPOS) {
            _systemVersion.substr(start).toInteger(_systemMajorVersion);
        }
    }

    //
    // Get host name.
    //
#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 1024> name;
    ::DWORD length = ::DWORD(name.size());
    if (::GetComputerNameW(name.data(), &length)) {
        _hostName.assign(name, length);
    }

#else

    // POSIX implementation.
    char name[1024];
    if (::gethostname(name, sizeof(name)) == 0) {
        name[sizeof(name) - 1] = '\0';
        _hostName.assignFromUTF8(name);
    }

#endif

    //
    // Get system memory page size
    //
#if defined(TS_WINDOWS)

    ::SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    _memoryPageSize = size_t(sysinfo.dwPageSize);

#else

    // POSIX implementation.
    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (pageSize > 0) {
        _memoryPageSize = size_t(pageSize);
    }

#endif

    //
    // Get support for specialized instructions.
    // Can be globally disabled using environment variables.
    //
    if (GetEnvironment(u"TS_NO_HARDWARE_ACCELERATION").empty()) {
        if (GetEnvironment(u"TS_NO_CRC32_INSTRUCTIONS").empty()) {
            #if defined(TS_LINUX) && defined(HWCAP_CRC32)
                _crcInstructions = tsCRC32IsAccelerated && (::getauxval(AT_HWCAP) & HWCAP_CRC32) != 0;
            #elif defined(TS_MAC)
                _crcInstructions = tsCRC32IsAccelerated && SysCtrlBool("hw.optional.armv8_crc32");
            #endif
        }
    }
}


//----------------------------------------------------------------------------
// Build a string representing the system on which the application runs.
//----------------------------------------------------------------------------

TS_REGISTER_FEATURE(u"system", u"System", ts::Features::ALWAYS, ts::SysInfo::GetSystemVersion);

ts::UString ts::SysInfo::GetSystemVersion()
{
    UString name(Instance().systemName());
    const UString version(Instance().systemVersion());
    if (!version.empty()) {
        name.format(u" (%s)", version);
    }
    const UChar* endian = nullptr;
    if constexpr (std::endian::native == std::endian::big) {
        endian = u"big";
    }
    else if constexpr (std::endian::native == std::endian::little) {
        endian = u"little";
    }
    else {
        endian = u"unknown";
    }
    name.format(u", on %s, %d-bit, %s-endian, page size: %d bytes", Instance().cpuName(), 8 * sizeof(void*), endian, Instance().memoryPageSize());
    return name;
}


//----------------------------------------------------------------------------
// Build a string describing the hardware accelerations on the system on which the application runs.
//----------------------------------------------------------------------------

TS_REGISTER_FEATURE(u"acceleration", u"Acceleration", ts::Features::ALWAYS, ts::SysInfo::GetAccelerations);

ts::UString ts::SysInfo::GetAccelerations()
{
    return UString::Format(u"CRC32: %s", UString::YesNo(Instance().crcInstructions()));
}


//----------------------------------------------------------------------------
// Build a string representing the compiler which was used to build TSDuck.
//----------------------------------------------------------------------------

TS_REGISTER_FEATURE(u"compiler", u"Compiler", ts::Features::ALWAYS, ts::SysInfo::GetCompilerVersion);

ts::UString ts::SysInfo::GetCompilerVersion()
{
    UString version;

    // Add compiler type and version.
#if defined(_MSC_FULL_VER)
    version.format(u"MSVC %02d.%02d.%05d", _MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000);
    #if defined(_MSC_BUILD)
    version.format(u".%02d", _MSC_BUILD);
    #endif
#elif defined(_MSC_VER)
    version.format(u"MSVC %02d.%02d", _MSC_VER / 100, _MSC_VER % 100);
    #if defined(_MSC_BUILD)
    version.format(u".%02d", _MSC_BUILD);
    #endif
#elif defined(__clang_version__)
    version.format(u"Clang %s", __clang_version__);
#elif defined(__llvm__) || defined(__clang__) || defined(__clang_major__)
    version.assign(u"Clang ");
    #if defined(__clang_major__)
    version.format(u"%d", __clang_major__);
    #endif
    #if defined(__clang_minor__)
    version.format(u".%d", __clang_minor__);
    #endif
    #if defined(__clang_patchlevel__)
    version.format(u".%d", __clang_patchlevel__);
    #endif
#elif defined(__GNUC__)
    version.format(u"GCC %d", __GNUC__);
    #if defined(__GNUC_MINOR__)
    version.format(u".%d", __GNUC_MINOR__);
    #endif
    #if defined(__GNUC_PATCHLEVEL__)
    version.format(u".%d", __GNUC_PATCHLEVEL__);
    #endif
#else
    version.assign(u"unknown compiler");
#endif

    // Add C++ revision level.
#if defined(_MSVC_LANG)
    // With MSVC, the standard macro __cplusplus is stuck at 199711 for obscure reasons.
    // The actual level of language standard is in the system-specific macro _MSVC_LANG.
    version.format(u", C++ std %04d.%02d", _MSVC_LANG / 100, _MSVC_LANG % 100);
#elif defined(__cplusplus)
    version.format(u", C++ std %04d.%02d", __cplusplus / 100, __cplusplus % 100);
#endif

    // Add debug and assertion modes.
    static constexpr bool use_debug =
#if defined(DEBUG)
        true;
#else
        false;
#endif
    bool use_assertions = false;
    assert(use_assertions = true);
    version.format(u", debug: %s, assertions: %s", UString::OnOff(use_debug), UString::OnOff(use_assertions));

    return version;
}

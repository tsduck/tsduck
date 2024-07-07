//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Cross-platforms portable base definitions for the TSDuck project.
//!
//!  This header file shall be included by all C++ compilation units
//!  in the TSDuck project. It provides common portable definitions.
//!  This file shall always be included first, before system headers and
//!  other TSDuck headers.
//!
//!  @b Important:
//!  This file shall be updated each time a new platform is used (processor,
//!  compiler, operating system). When recompiling this header on a new
//!  platform, many errors are generated (explicit @c @#error directives).
//!  These errors will guide you on the required additions for the new
//!  platform.
//!
//!  @section platforms Platform definitions
//!
//!  This header file conditionally defines several symbols (macros) which
//!  describe the platform (processor, compiler, operating system). These
//!  symbols can be used for conditional compilation.
//!
//!  The following environments are described:
//!
//!  @li Compiler: See @link TS_GCC @endlink, @link TS_MSC @endlink, etc.
//!  @li Operating system: See @link TS_LINUX @endlink, @link TS_WINDOWS @endlink, etc.
//!  @li Byte ordering: See @link TS_LITTLE_ENDIAN @endlink and @link TS_BIG_ENDIAN @endlink.
//!  @li Processor architecture: See @link TS_I386 @endlink, @link TS_X86_64 @endlink, etc.
//!
//----------------------------------------------------------------------------

#pragma once

// Include an "empty" header file.
// Updated with preconfigured constants in custom installations.
#include "tsPreConfiguration.h"


//----------------------------------------------------------------------------
// Unified compiler naming: TS_GCC, TS_MSC (Microsoft C)
//----------------------------------------------------------------------------

#if defined(DOXYGEN)
    //!
    //! Defined when the compiler is GCC, also known as "GNU C", or a GCC-compatible compiler.
    //!
    #define TS_GCC
    //!
    //! Defined when the compiler is exactly GCC, also known as "GNU C", not a GCC-compatible compiler.
    //!
    #define TS_GCC_ONLY
    //!
    //! Defined when the compiler is LLVM (clang).
    //! In that case, TS_GCC is also defined since LLVM is reasonably compatible with GCC.
    //!
    #define TS_LLVM
    //!
    //! Defined when the compiler is Microsoft C/C++, the default compiler
    //! in the Microsoft Visual Studio environment. Also used on command line
    //! and batch file as the @c cl command.
    //!
    #define TS_MSC
    //
#elif defined(__llvm__) || defined(__clang__)
    #if !defined(TS_LLVM)
        #define TS_LLVM 1
    #endif
    #if !defined(TS_GCC)
        #define TS_GCC 1
    #endif
#elif defined(__GNUC__)
    #if !defined(TS_GCC)
        #define TS_GCC 1
    #endif
    #if !defined(TS_GCC_ONLY)
        #define TS_GCC_ONLY 1
    #endif
#elif defined(_MSC_VER)
    #if !defined(TS_MSC)
        #define TS_MSC 1
    #endif
#else
    #error "New unknown compiler, please update tsPlatform.h"
#endif

#if defined(DOXYGEN)
    //!
    //! GCC version, encoded as an integer.
    //! Example: 40801 for GCC 4.8.1.
    //! Undefined when the compiler is not GCC or its version is unknown.
    //!
    #define TS_GCC_VERSION
    //
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
    #define TS_GCC_VERSION ((10000 * __GNUC__) + (100 * __GNUC_MINOR__) + (__GNUC_PATCHLEVEL__ % 100))
#endif

#if defined(DOXYGEN)
    //!
    //! Level of C++ language, same as predefined @a __cplusplus, but safe with MSVC.
    //!
    //! Microsoft-specific ways of signaling the language level: With Visual Studio 2017,
    //! there are flags such as /std:c++14 to specify the level of language. However,
    //! __cplusplus is still set to 199711L. It is unclear if this is a bug or if __cplusplus
    //! is not set because the C++11/14/17 standards all not super-completely implemented.
    //! The Microsoft-specific symbol _MSVC_LANG is defined to describe a "good-enough"
    //! level of standard which is fine for us.
    //!
    //! The current version of TSDuck requires C++17 at least (GCC 8, clang 5, Visual Studio 2017 15.8).
    //!
    #define TS_CPLUSPLUS
    //!
    //! Defined when the compiler is compliant with C++17.
    //!
    #define TS_CXX17
    //!
    //! Defined when the compiler is compliant with C++20.
    //!
    #define TS_CXX20
    //!
    //! Defined when the compiler is compliant with C++23.
    //!
    #define TS_CXX23
    //
#else
    #if !defined(TS_CPLUSPLUS)
        #if defined(__cplusplus)
            #define TS_CPLUSPLUS __cplusplus
        #else
            #define TS_CPLUSPLUS 0L
        #endif
    #endif
    #if defined(_MSVC_LANG) && _MSVC_LANG > TS_CPLUSPLUS
        #undef TS_CPLUSPLUS
        #define TS_CPLUSPLUS _MSVC_LANG
    #endif
    #if TS_CPLUSPLUS >= 201703L && !defined(TS_CXX17)
        #define TS_CXX17 1
    #endif
    #if TS_CPLUSPLUS >= 202002L && !defined(TS_CXX20)
        #define TS_CXX20 1
    #endif
    #if TS_CPLUSPLUS >= 202302L && !defined(TS_CXX23)
        #define TS_CXX23 1
    #endif
#endif

#if defined(DOXYGEN) || !defined(TS_CXX20)
    //!
    //! Defined when the != operator shall be defined in addition to ==.
    //!
    //! In C++20, != operators are implicitly derived from the corresponding == operators.
    //! Defining the != operator when == is already defined is not only useless,
    //! it also creates "ambiguous operator" errors because tests such as "a != b"
    //! become ambiguous when a and b are not const (the exact profile of !=).
    //! In that case, the compiler cannot choose between the explicitly defined
    //! != and the implicit one which is derived from ==.
    //!
    #define TS_NEED_UNEQUAL_OPERATOR 1
#endif

// TSDuck now requests to use C++17 at least.
#if !defined(TS_CXX17)
    #if defined(TS_MSC)
        #error "TSDuck requires C++17 at least, use /std:c++17 or higher"
    #else
        #error "TSDuck requires C++17 at least, use -std=c++17 or higher"
    #endif
#endif


//----------------------------------------------------------------------------
// Unified O/S naming: TS_LINUX, TS_WINDOWS, etc
//----------------------------------------------------------------------------

#if defined(DOXYGEN)
    //!
    //! Defined when compiled for a Microsoft Windows target platform.
    //!
    #define TS_WINDOWS
    //!
    //! Defined when compiled for any flavor of UNIX target platforms.
    //!
    //! This symbol comes in addition to the specific symbol for the
    //! target platform (@link TS_LINUX @endlink, etc.)
    //!
    #define TS_UNIX
    //!
    //! Defined when compiled for a Linux target platform.
    //!
    #define TS_LINUX
    //!
    //! Defined when compiled for a BSD target platform (FreeBSD, OpenBSD, NetBSD, DragonFlyBSD).
    //!
    #define TS_BSD
    //!
    //! Defined when compiled for a FreeBSD target platform.
    //!
    #define TS_FREEBSD
    //!
    //! Defined when compiled for an OpenBSD target platform.
    //!
    #define TS_OPENBSD
    //!
    //! Defined when compiled for a NetBSD target platform.
    //!
    #define TS_NETBSD
    //!
    //! Defined when compiled for a DragonFlyBSD target platform.
    //!
    #define TS_DRAGONFLYBSD
    //!
    //! Defined when compiled for a macOS target platform.
    //!
    #define TS_MAC
     //!
    //! Defined when compiled for an IBM AIX target platform.
    //!
    #define TS_AIX
    //!
    //! Defined when compiled for a Sun Solaris target platform.
    //!
    #define TS_SOLARIS
    //!
    //! Defined when compiled for a Cygwin target platform.
    //!
    #define TS_CYGWIN
    //!
    //! Defined when compiled for an Android target platform.
    //!
    #define TS_ANDROID

#elif defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    #if !defined(TS_WINDOWS)
        #define TS_WINDOWS 1
    #endif
#elif defined(__FreeBSD__) || defined(TS_FREEBSD)
    #if !defined(TS_FREEBSD)
        #define TS_FREEBSD 1
    #endif
#elif defined(__DragonFly__) || defined(TS_DRAGONFLYBSD)
    #if !defined(TS_DRAGONFLYBSD)
        #define TS_DRAGONFLYBSD 1
    #endif
#elif defined(__OpenBSD__) || defined(TS_OPENBSD)
    #if !defined(TS_OPENBSD)
        #define TS_OPENBSD 1
    #endif
#elif defined(__NetBSD__) || defined(TS_NETBSD)
    #if !defined(TS_NETBSD)
        #define TS_NETBSD 1
    #endif
#elif defined(__gnu_linux__) || defined(TS_LINUX) || defined(__linux__) || defined(linux)
    #if !defined(TS_LINUX)
        #define TS_LINUX 1
    #endif
#elif defined(__APPLE__)
    #if !defined(TS_MAC)
        #define TS_MAC 1
    #endif
#elif defined(_AIX) || defined(TS_AIX)
    #if !defined(TS_AIX)
        #define TS_AIX 1
    #endif
#elif defined(__CYGWIN__) || defined(TS_CYGWIN)
    #if !defined(TS_CYGWIN)
        #define TS_CYGWIN 1
    #endif
#elif defined(__sun) || defined(TS_SOLARIS)
    #if !defined(TS_SOLARIS)
        #define TS_SOLARIS 1
    #endif
#else
    #error "New unknown operating system, please update tsPlatform.h"
#endif

#if defined(__ANDROID__) && !defined(TS_ANDROID)
    // Android comes in addition to Linux.
    #define TS_ANDROID 1
#endif

#if !defined(TS_BSD) && (defined(TS_FREEBSD) || defined(TS_DRAGONFLYBSD) || defined(TS_OPENBSD) || defined(TS_NETBSD))
    #define TS_BSD 1
#endif
#if !defined(TS_UNIX) && (defined(__unix) || defined(__unix__) || defined(TS_LINUX) || defined(TS_BSD) || defined(TS_MAC) || defined(TS_AIX) || defined(TS_CYGWIN) || defined(TS_SOLARIS))
    #define TS_UNIX 1
#endif


//----------------------------------------------------------------------------
// Unified processor naming
//----------------------------------------------------------------------------

#if defined(DOXYGEN)
    //!
    //! Defined when compiled for a little-endian or LSB-first target platform.
    //!
    #define TS_LITTLE_ENDIAN
    //!
    //! Defined when compiled for a big-endian or MSB-first target platform.
    //!
    #define TS_BIG_ENDIAN
    //!
    //! Defined to @c true when compiled for a little-endian or LSB-first target platform, @a false otherwise.
    //!
    #define TS_LITTLE_ENDIAN_BOOL true/false
    //!
    //! Defined to @c true when compiled for a big-endian or MSB-first target platform, @a false otherwise.
    //!
    #define TS_BIG_ENDIAN_BOOL true/false
    //!
    //! Number of bits in an address (or a pointer or a size_t).
    //!
    #define TS_ADDRESS_BITS 16, 32, 64, etc.
    //!
    //! Defined when the target processor architecture is Intel IA-32, also known as x86.
    //!
    #define TS_I386
    //!
    //! Defined when the target processor architecture is the 64-bit extension of the
    //! IA-32 architecture, also known as AMD-64 or Intel x86-64.
    //!
    #define TS_X86_64
    //!
    //! Defined when the target processor architecture is 32-bit ARM.
    //!
    #define TS_ARM32
    //!
    //! Defined when the target processor architecture is 64-bit ARM.
    //!
    #define TS_ARM64
    //!
    //! Defined when the target processor architecture is 32-bit Power PC.
    //!
    #define TS_POWERPC
    //!
    //! Defined when the target processor architecture is 64-bit Power PC.
    //!
    #define TS_POWERPC64
    //!
    //! Defined when the target processor architecture is Sun SPARC architecture.
    //!
    #define TS_SPARC
    //!
    //! Defined when the target processor architecture is 32-bit MIPS architecture.
    //!
    #define TS_MIPS
    //!
    //! Defined when the target processor architecture is 64-bit MIPS architecture.
    //!
    #define TS_MIPS64
    //!
    //! Defined when the target processor architecture is 64-bit RISC-V.
    //!
    #define TS_RISCV64
    //!
    //! Defined when the target processor architecture is S390X.
    //!
    #define TS_S390X

#elif defined(__i386__) || defined(TS_I386) || defined(_M_IX86)
    #if !defined(TS_I386)
        #define TS_I386 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__amd64) || defined(__amd64__) || defined(__x86_64__) || defined(TS_X86_64) || defined(_M_X64)
    #if !defined(TS_X86_64)
        #define TS_X86_64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__ia64__) || defined(_M_IA64)
    #if !defined(TS_IA64)
        #define TS_IA64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__aarch64__) || defined(__arm64__)
    #if !defined(TS_ARM64)
        #define TS_ARM64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__arm__)
    #if !defined(TS_ARM32)
        #define TS_ARM32 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__sparc__)
    #if !defined(TS_SPARC)
        #define TS_SPARC 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        // To be fixed for SPARC 64
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__powerpc64__)
    #if !defined(TS_POWERPC64)
        #define TS_POWERPC64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__powerpc__)
    #if !defined(TS_POWERPC)
        #define TS_POWERPC 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__mips64)
    #if !defined(TS_MIPS64)
        #define TS_MIPS64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__mips__)
    #if !defined(TS_MIPS)
        #define TS_MIPS 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 64)
    #if !defined(TS_RISCV64)
        #define TS_RISCV64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__s390x__)
    #if !defined(TS_S390X)
        #define TS_S390X 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#else
    #error "New unknown processor, please update tsPlatform.h"
#endif

// Untested platform warnings.
// Should really use "#warning" instead of "#error" but #warning is a
// GCC extension while #error is standard.

#if !defined(TS_LINUX) && !defined(TS_WINDOWS) && !defined(TS_MAC) && !defined(TS_FREEBSD) && !defined(TS_DRAGONFLYBSD) && !defined(TS_OPENBSD) && !defined(TS_NETBSD)
    #error "TSDuck has been tested on Linux, macOS, FreeBSD, OpenBSD, NetBSD, DragonFlyBSD and Windows only, review this code"
#endif

#if !defined(TS_GCC) && !defined(TS_LLVM) && !defined(TS_MSC)
    #error "TSDuck has been tested with GCC, LLVM (Clang) and MSVC compilers only, review this code"
#endif

// Byte order

// These architectures are mono-endian. Only one possible endianness.
#if (defined(TS_I386) || defined(TS_X86_64) || defined(TS_IA64) || defined(TS_ALPHA) || defined(TS_RISCV64)) && !defined(TS_LITTLE_ENDIAN)
    #define TS_LITTLE_ENDIAN 1
#elif (defined(TS_SPARC) || defined(TS_S390X)) && !defined(TS_BIG_ENDIAN)
    #define TS_BIG_ENDIAN 1
#endif

// These architectures are bi-endian. Need to check from the compiler.
#if (defined(TS_ARM32) || defined(TS_ARM64) || defined(TS_MIPS) || defined(TS_MIPS64) || defined(TS_POWERPC) || defined(TS_POWERPC64)) && !defined(TS_LITTLE_ENDIAN) && !defined(TS_BIG_ENDIAN)
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define TS_LITTLE_ENDIAN 1
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define TS_BIG_ENDIAN 1
    #elif defined(__LITTLE_ENDIAN__)
        #define TS_LITTLE_ENDIAN 1
    #elif defined(__BIG_ENDIAN__)
        #define TS_BIG_ENDIAN 1
    #else
        #error "unknow endianness, __BYTE_ORDER__ not defined"
    #endif
#endif

#if defined(TS_LITTLE_ENDIAN)
    #define TS_LITTLE_ENDIAN_BOOL true
    #define TS_BIG_ENDIAN_BOOL false
#elif defined(TS_BIG_ENDIAN)
    #define TS_LITTLE_ENDIAN_BOOL false
    #define TS_BIG_ENDIAN_BOOL true
#else
    #error "unknow endianness, please update this header file"
#endif

#if !defined(TS_ADDRESS_BITS)
    #error "unknow address size, please update this header file"
#endif

//!
//! Defined when the CPU requires strict memory alignment (address must be a multiple of the data size).
//!
#if (defined(DOXYGEN) || defined(__ARM_ARCH_5TEJ__)) && !defined(TS_STRICT_MEMORY_ALIGN)
    #define TS_STRICT_MEMORY_ALIGN 1
#endif

//!
//! Define TS_NO_ARM_CRC32_INSTRUCTIONS from the command line if you want to disable the usage of Arm64 CRC32 instructions.
//!
#if defined(DOXYGEN)
    #define TS_NO_ARM_CRC32_INSTRUCTIONS
#endif


//----------------------------------------------------------------------------
// Windows oddities.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS) && !defined(DOXYGEN)
    //
    // Workaround for MSVC runtime compatibility mess.
    //
    // With MSVC compiler version 19.40.33811 and mscvp140.dll 14.40.33810.00, the class std::mutex has a constexpr
    // constructor. No explicit constructor is called, the mutex memory is correctly initialized. However, when
    // this application code is executed in the context of an older mscvp140.dll (as brought by another application
    // for instance), the older runtime crashes when trying to acquire the mutex, because the mutex memory area is
    // not properly initialized, according to this older runtime.
    //
    // The problem originally appeared on GitHub Actions windows-2022 runner version 2.317.0, image version 20240603.1.0.
    // But it expanded to other environments such as Java. If the installed JVM contains an older mscvp140.dll, the
    // Java application crashes when loading the TSDuck DLL in the JNI initialization.
    //
    // The symbol below disables the constexpr constructor and reverts to the older behaviour. Thus, the compiled
    // TSDuck code becomes compatible with older and newer mscvp140.dll. This definition should be removed some day,
    // when all environments are upgraded to a recent mscvp140.dll, which means probably never...
    //
    #define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 1
#endif


//----------------------------------------------------------------------------
// Static linking.
//----------------------------------------------------------------------------

#if defined(DOXYGEN)
    //!
    //! User-defined macro to enable static linking of plugins.
    //!
    //! If TSDUCK_STATIC_PLUGINS is defined, typically on the compilation command line,
    //! all plugins are linked statically. Dynamic loading of plugins is disabled.
    //! @hideinitializer
    //!
    #define TSDUCK_STATIC_PLUGINS 1
    //!
    //! Applications which link against the TSDuck static library.
    //!
    //! Applications which link against the TSDuck static library should define
    //! TSDUCK_STATIC_LIBRARY. This symbol can be used to force external references.
    //!
    //! TSDUCK_STATIC_LIBRARY enforces TSDUCK_STATIC_PLUGINS.
    //! @hideinitializer
    //!
    #define TSDUCK_STATIC_LIBRARY 1
    //!
    //! User-defined macro to enable full static linking.
    //!
    //! If TSDUCK_STATIC is defined, typically on the compilation command line,
    //! the code is compiled and linked statically, including system libraries.
    //! On Linux, this is not recommended since a few features such as IP address
    //! resolution are disabled.
    //!
    //! TSDUCK_STATIC enforces TSDUCK_STATIC_LIBRARY and TSDUCK_STATIC_PLUGINS.
    //! @hideinitializer
    //!
    #define TSDUCK_STATIC 1

#else

    // Full static linking enforces the usage of TSDuck static library.
    #if defined(TSDUCK_STATIC) && !defined(TSDUCK_STATIC_LIBRARY)
        #define TSDUCK_STATIC_LIBRARY 1
    #endif

    // Linking against the TSDuck static library enforces statically linked plugins.
    #if defined(TSDUCK_STATIC_LIBRARY) && !defined(TSDUCK_STATIC_PLUGINS)
        #define TSDUCK_STATIC_PLUGINS 1
    #endif

#endif // DOXYGEN


//----------------------------------------------------------------------------
// Always enforce assertions when TS_KEEP_ASSERTIONS is defined
//----------------------------------------------------------------------------

#if defined(NDEBUG) && defined(TS_KEEP_ASSERTIONS)
    #undef NDEBUG
#endif


//----------------------------------------------------------------------------
// Macro parameter to string transformation
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)
    #define TS_STRINGIFY1(x) #x
    #define TS_USTRINGIFY2(x) u ## x
    #define TS_USTRINGIFY1(x) TS_USTRINGIFY2(#x)
#endif

//!
//! @hideinitializer
//! This macro transforms the @e value of a macro parameter into the equivalent string literal.
//!
//! This is a very specific macro. It is typically used only inside the definition of
//! another macro. It is similar to the @# token in the preprocessor but has a slightly
//! different effect. The @# token transforms the @e text of a macro parameter into a
//! string while TS_STRINGIFY transforms the @e value of a macro parameter into a
//! string, after all preprocessing substitutions.
//!
//! The following example illustrates the difference between the @# token and TS_STRINGIFY:
//!
//! @code
//! #define P1(v) printf("#parameter:   %s = %d\n", #v, v)
//! #define P2(v) printf("TS_STRINGIFY: %s = %d\n", TS_STRINGIFY(v), v)
//! ....
//! #define X 1
//! P1(X);
//! P2(X);
//! @endcode
//!
//! This will print:
//!
//! @code
//! #parameter:   X = 1
//! TS_STRINGIFY: 1 = 1
//! @endcode
//!
#define TS_STRINGIFY(x) TS_STRINGIFY1(x)

//!
//! @hideinitializer
//! This macro transforms the @e value of a macro parameter into the equivalent 16-bit Unicode string literal.
//!
//! This macro is equivalent to TS_STRINGIFY() except that the string literal is of the for @c u"..." instead of @c "..."
//! @see TS_STRINGIFY()
//!
#define TS_USTRINGIFY(x) TS_USTRINGIFY1(x)


//----------------------------------------------------------------------------
// Unique name generation
//----------------------------------------------------------------------------

//! @cond nodoxygen
#define TS_UNIQUE_NAME1(a,b) a##b
#define TS_UNIQUE_NAME2(a,b) TS_UNIQUE_NAME1(a,b)
//! @endcond

//!
//! @hideinitializer
//! This macro generates a unique name based on the source line number.
//! It is typically used in more complex macro definitions. These complex
//! macros should be defined on one single line. All invocations of TS_UNIQUE_NAME
//! generate the same symbol when placed one the same source line.
//! @param prefix Prefix for the symbol. Using different prefixes, several
//! distinct unique identifiers can be used on the same line.
//!
#define TS_UNIQUE_NAME(prefix) TS_UNIQUE_NAME2(prefix,__LINE__)


//----------------------------------------------------------------------------
// Compiler warnings
//----------------------------------------------------------------------------

#if defined(DOXYGEN)
    //!
    //! Helper macro for the C++11 keyword @c _Pragma.
    //!
    //! With the C++11 standard, the keywork @c _Pragma uses a string literal.
    //! However, Visual C++ does not support it as of Visual Studio 2019.
    //! Instead, it uses the non-standard keyword @c __pragma with a plain directive (not a string).
    //! @param directive The directive for the pragma. Plain directive, not a string literal.
    //!
    //! Examples (depending on compilers):
    //! @code
    //! TS_PRAGMA(clang diagnostic push)
    //! TS_PRAGMA(GCC diagnostic push)
    //! TS_PRAGMA(warning(push))
    //! @endcode
    //!
    #define TS_PRAGMA(directive)
    //!
    //! Save the compiler's warnings reporting.
    //! @see TS_POP_WARNING
    //!
    #define TS_PUSH_WARNING()
    //!
    //! Restore the compiler's warnings reporting from a previous TS_PUSH_WARNING().
    //! @see TS_PUSH_WARNING
    //!
    #define TS_POP_WARNING()
    //!
    //! Disable a warning with the LLVM/clang compiler (does nothing on other compilers).
    //! @param name Warning name, as used after command line option @c -W
    //!
    #define TS_LLVM_NOWARNING(name)
    //!
    //! Disable a warning with the GCC compiler (does nothing on other compilers).
    //! @param name Warning name, as used after command line option @c -W
    //!
    #define TS_GCC_NOWARNING(name)
    //!
    //! Disable a warning with the Microsoft Visual C++ compiler (does nothing on other compilers).
    //! @param num Warning number.
    //!
    #define TS_MSC_NOWARNING(num)

#else

    // Use a two-step macro to allow stringification of parameters for pragma.
    #if defined(TS_MSC)
        #define TS_PRAGMA1_(s) __pragma(s)
    #else
        #define TS_PRAGMA1_(s) _Pragma(#s)
    #endif
    #define TS_PRAGMA(s) TS_PRAGMA1_(s)

    #if defined(TS_LLVM)
        #define TS_PUSH_WARNING()       TS_PRAGMA(clang diagnostic push)
        #define TS_POP_WARNING()        TS_PRAGMA(clang diagnostic pop)
        #define TS_LLVM_NOWARNING(name) TS_PRAGMA(clang diagnostic ignored TS_STRINGIFY(-W##name))
        #define TS_GCC_NOWARNING(name)
        #define TS_MSC_NOWARNING(num)
    #elif defined(TS_GCC)
        #define TS_PUSH_WARNING()       TS_PRAGMA(GCC diagnostic push)
        #define TS_POP_WARNING()        TS_PRAGMA(GCC diagnostic pop)
        #define TS_LLVM_NOWARNING(name)
        #define TS_GCC_NOWARNING(name)  TS_PRAGMA(GCC diagnostic ignored TS_STRINGIFY(-W##name))
        #define TS_MSC_NOWARNING(num)
        TS_GCC_NOWARNING(pragmas)       // ignore unknown recent warnings on older compilers
    #elif defined(TS_MSC)
        #define TS_PUSH_WARNING()       TS_PRAGMA(warning(push))
        #define TS_POP_WARNING()        TS_PRAGMA(warning(pop))
        #define TS_LLVM_NOWARNING(name)
        #define TS_GCC_NOWARNING(name)
        #define TS_MSC_NOWARNING(num)   TS_PRAGMA(warning(disable:num))
    #else
        #define TS_PUSH_NOWARNING()
        #define TS_POP_NOWARNING()
        #define TS_LLVM_NOWARNING(name)
        #define TS_GCC_NOWARNING(name)
        #define TS_MSC_NOWARNING(num)
    #endif

#endif // DOXYGEN

//
// Disable some warnings, application-wide, for various compilers.
//
TS_GCC_NOWARNING(unused-parameter)                // Unused parameters are frequent with overrides.
#if defined(TS_ARM32)
    TS_GCC_NOWARNING(psabi)
#endif

TS_LLVM_NOWARNING(unknown-warning-option)         // Ignore recent warnings when unknown in an older version.
TS_LLVM_NOWARNING(unused-parameter)               // Unused parameters are frequent with overrides.
TS_LLVM_NOWARNING(global-constructors)            // Do not warn about static/global objects being constructed.
TS_LLVM_NOWARNING(exit-time-destructors)          // Do not warn about static/global objects being destructed.
TS_LLVM_NOWARNING(covered-switch-default)         // Allow "default" in "switch" after all enum values to catch invalid binary values.
TS_LLVM_NOWARNING(sign-conversion)                // Too many occurences since pointer arithmetics is signed, opposite from size_t.
TS_LLVM_NOWARNING(padded)                         // Do not care if padding is required between class fields.
TS_LLVM_NOWARNING(reserved-id-macro)              // We sometimes use underscores at the beginning of identifiers.
TS_LLVM_NOWARNING(reserved-identifier)            // Identifier '_Xxx' is reserved because it starts with '_' followed by a capital letter.
TS_LLVM_NOWARNING(c++98-compat-pedantic)          // Require C++17, no need for C++98 compatibility.
TS_LLVM_NOWARNING(c++2a-compat)                   // Compatibility with C++2a is not yet a concern.
TS_LLVM_NOWARNING(documentation-unknown-command)  // Some valid doxygen directives are unknown to clang.
TS_LLVM_NOWARNING(unsafe-buffer-usage)            // "unsafe pointer arithmetic" (new with clang 16) => we know what we are doing here.


TS_MSC_NOWARNING(4100)  // unreferenced formal parameter
TS_MSC_NOWARNING(4189)  // local variable is initialized but not referenced
TS_MSC_NOWARNING(4251)  // 'classname' : class 'std::vector<_Ty>' needs to have dll-interface to be used by clients of class 'classname'
TS_MSC_NOWARNING(4275)  // non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class 'std::_Container_base_aux_alloc_real<_Alloc>'
TS_MSC_NOWARNING(4355)  // 'this' : used in base member initializer list
TS_MSC_NOWARNING(4365)  // conversion from 'type1' to 'type2', signed/unsigned mismatch
TS_MSC_NOWARNING(4619)  // disablement of warning that doesn't exist
TS_MSC_NOWARNING(4623)  // default constructor was implicitly defined as deleted
TS_MSC_NOWARNING(4625)  // copy constructor was implicitly defined as deleted
TS_MSC_NOWARNING(4626)  // assignment operator was implicitly defined as deleted
TS_MSC_NOWARNING(4627)  // 'header_file': skipped when looking for precompiled header use
TS_MSC_NOWARNING(4628)  // digraphs not supported with -Ze. Character sequence '<:' not interpreted as alternate token for '['
TS_MSC_NOWARNING(4820)  // 'n' bytes padding added after data member 'nnnnn'
TS_MSC_NOWARNING(5026)  // move constructor was implicitly defined as deleted
TS_MSC_NOWARNING(5039)  // pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception
TS_MSC_NOWARNING(5220)  // a non-static data member with a volatile qualified type no longer implies that compiler generated copy/move constructors and copy/move assignment operators are not trivial

// The following ones should really be informational instead of warning:
TS_MSC_NOWARNING(4371)  // layout of class may have changed from a previous version of the compiler due to better packing of member 'xxxx'
TS_MSC_NOWARNING(4514)  // unreferenced inline function has been removed
TS_MSC_NOWARNING(4571)  // catch (...) semantics changed since Visual C++ 7.1; structured exceptions(SEH) are no longer caught
TS_MSC_NOWARNING(4710)  // 'xxx' : function not inlined
TS_MSC_NOWARNING(4711)  // function 'xxx' selected for automatic inline expansion
TS_MSC_NOWARNING(5045)  // Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified


//----------------------------------------------------------------------------
// Commonly used C++ headers.
//----------------------------------------------------------------------------

#include "tsBeforeStandardHeaders.h"
#include <string>
#include <vector>
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <bitset>
#include <optional>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <limits>
#include <locale>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <filesystem>
#include <typeinfo>
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <cctype>
#include <cwctype>
#include <cstddef>
#include <climits>
#include <cmath>
#include <fcntl.h>
#if defined(TS_WINDOWS)
    #include <windows.h>
#endif
#if defined(TS_UNIX)
    #include <unistd.h>
    #include <glob.h>
    #include <sys/mman.h>
#endif
#include "tsAfterStandardHeaders.h"


//----------------------------------------------------------------------------
// Basic preprocessing features
//----------------------------------------------------------------------------

//!
//! Attribute to explicitly disable optimization in a function.
//!
//! Example:
//! @code
//! void f() TS_NO_OPTIMIZE;
//! @endcode
//!
#if defined(DOXYGEN)
    #define TS_NO_OPTIMIZE platform_specific
#elif defined(TS_MSC)
    #define TS_NO_OPTIMIZE
#elif defined(TS_LLVM)
    #define TS_NO_OPTIMIZE __attribute__((optnone))
#elif defined(TS_GCC)
    #define TS_NO_OPTIMIZE __attribute__((optimize("-O0")))
#else
    #error "New unknown compiler, please update TS_NO_OPTIMIZE in tsPlatform.h"
#endif

//!
//! Definition of the name of the current function.
//! This is typically __func__ but recent compilers have "pretty" names for C++.
//!
#if defined(DOXYGEN)
    #define TS_FUNCTION
#elif defined(TS_GCC) && __GNUC__ >= 3
    #define TS_FUNCTION __PRETTY_FUNCTION__
#elif defined(TS_MSC)
    #define TS_FUNCTION __FUNCDNAME__
#else
    #define TS_FUNCTION __func__
#endif

//!
//! String version of __LINE__
//!
#define TS_SLINE TS_STRINGIFY(__LINE__)

//!
//! @hideinitializer
//! Attribute to declare a class or function from tsduck.dll on Windows.
//!
//! When building tsduck.dll on Windows, define _TSDUCKDLL_IMPL in the project options.
//! When building a project which references tsduck.dll, define _TSDUCKDLL_USE.
//! All API located inside tsduck.dll shall be prefixed by TSDUCKDLL in headers.
//! This prefix exports the API when building the DLL and imports the API
//! when used in an application.
//!
#if defined(TS_WINDOWS) && defined(_TSDUCKDLL_IMPL)
    #define TSDUCKDLL __declspec(dllexport)
#elif defined(TS_WINDOWS) && defined(_TSDUCKDLL_USE)
    #define TSDUCKDLL __declspec(dllimport)
#else
    #define TSDUCKDLL
#endif

//!
//! A macro to disable object copy in the declaration of a class.
//! The copy and move constructors and assignments are explicitly deleted.
//! @param classname Name of the enclosing class.
//!
#define TS_NOCOPY(classname)                        \
    private:                                        \
        classname(classname&&) = delete;            \
        classname(const classname&) = delete;       \
        classname& operator=(classname&&) = delete; \
        classname& operator=(const classname&) = delete

//!
//! A macro to disable default constructor and object copy in the declaration of a class.
//! The default, copy and move constructors and assignments are explicitly deleted.
//! @param classname Name of the enclosing class.
//!
#define TS_NOBUILD_NOCOPY(classname)                \
    private:                                        \
        classname() = delete;                       \
        classname(classname&&) = delete;            \
        classname(const classname&) = delete;       \
        classname& operator=(classname&&) = delete; \
        classname& operator=(const classname&) = delete

//!
//! A macro to disable default constructors in the declaration of a class.
//! The default, copy and move constructors are explicitly deleted.
//! @param classname Name of the enclosing class.
//!
#define TS_NO_DEFAULT_CONSTRUCTORS(classname) \
    private:                                  \
        classname() = delete;                 \
        classname(classname&&) = delete;      \
        classname(const classname&) = delete

//!
//! A macro to declare the default assignment operators in the declaration of a class.
//! @param classname Name of the enclosing class.
//!
#define TS_DEFAULT_ASSIGMENTS(classname)                  \
    public:                                               \
        /** @cond nodoxygen */                            \
        classname& operator=(classname&&) = default;      \
        classname& operator=(const classname&) = default  \
        /** @endcond */

//!
//! A macro to declare the default copy and move constructors and assignment operators in the declaration of a class.
//! @param classname Name of the enclosing class.
//!
#define TS_DEFAULT_COPY_MOVE(classname)                   \
    public:                                               \
        /** @cond nodoxygen */                            \
        classname(classname&&) = default;                 \
        classname(const classname&) = default;            \
        classname& operator=(classname&&) = default;      \
        classname& operator=(const classname&) = default  \
        /** @endcond */

//!
//! A macro to apply the C++ "rule of five" in the declaration of a class.
//! An explicit virtual destructor is declared. The copy and move constructors and assignments are defaulted.
//! @param classname Name of the enclosing class.
//! @param dtor_attributes Post attributes for the destructor.
//!
#define TS_RULE_OF_FIVE(classname, dtor_attributes)       \
    public:                                               \
        /** @cond nodoxygen */                            \
        classname(classname&&) = default;                 \
        classname(const classname&) = default;            \
        classname& operator=(classname&&) = default;      \
        classname& operator=(const classname&) = default; \
        virtual ~classname() dtor_attributes              \
        /** @endcond */

//!
//! A macro to declare the basic operators in the declaration of an interface class.
//! @param classname Name of the enclosing class.
//!
#define TS_INTERFACE(classname)                           \
    public:                                               \
        /** @cond nodoxygen */                            \
        classname() = default;                            \
        classname(classname&&) = default;                 \
        classname(const classname&) = default;            \
        classname& operator=(classname&&) = default;      \
        classname& operator=(const classname&) = default; \
        virtual ~classname()                              \
        /** @endcond */

//!
//! A macro to declare an unequal operator in the declaration of a class.
//! Before C++20, define an inline != operator based on the == operator.
//! In C++20 and beyond, != is implicitly derived from the corresponding ==
//! and this macro does nothing.
//! @param classname Name of the enclosing class.
//!
#if defined(TS_NEED_UNEQUAL_OPERATOR) && !defined(DOXYGEN)
    #define TS_UNEQUAL_OPERATOR(classname) bool operator!=(const classname& other) const { return ! operator==(other); }
#else
    #define TS_UNEQUAL_OPERATOR(classname)
#endif


//----------------------------------------------------------------------------
// Source code identification.
//----------------------------------------------------------------------------

//!
//! This macro generates a reference to some address (string literal, external symbol, etc.)
//! The reference is a unique static symbol which is otherwise unused.
//! The difficulty is to make sure that the compiler will not optimize away this data (it is local and unused).
//! @param suffix Some unique suffix if the macro is invoked several times on the same line.
//! @param addr An address to reference (string literal, external symbol, etc.)
//! @hideinitializer
//!
#if defined(TS_GCC)
    #define TS_STATIC_REFERENCE(suffix, addr) \
        static __attribute__ ((used)) volatile const void* volatile const TS_UNIQUE_NAME(ref##suffix) = (addr)
#else
    // Keep this definition on one line because of TS_UNIQUE_NAME.
    #define TS_STATIC_REFERENCE(suffix, addr) \
        static volatile const void* volatile const TS_UNIQUE_NAME(ref##suffix) = (addr); static volatile const void* volatile const TS_UNIQUE_NAME(ref2##suffix) = &TS_UNIQUE_NAME(ref##suffix)
#endif


//----------------------------------------------------------------------------
//! TSDuck namespace, containing all TSDuck classes and functions.
//----------------------------------------------------------------------------

namespace ts {

    // Some common pointer types, typically for casting.
    using char_ptr = char*;                    //!< Pointer to @c char
    using int8_ptr = int8_t*;                  //!< Pointer to @c int8_t
    using int16_ptr = int16_t*;                //!< Pointer to @c int16_t
    using int32_ptr = int32_t*;                //!< Pointer to @c int32_t
    using int64_ptr = int64_t*;                //!< Pointer to @c int64_t
    using uint8_ptr = uint8_t*;                //!< Pointer to @c uint8_t
    using uint16_ptr = uint16_t*;              //!< Pointer to @c uint16_t
    using uint32_ptr = uint32_t*;              //!< Pointer to @c uint32_t
    using uint64_ptr = uint64_t*;              //!< Pointer to @c uint64_t
    using const_char_ptr = const char*;        //!< Pointer to @c const char
    using const_int8_ptr = const int8_t*;      //!< Pointer to @c const int8_t
    using const_int16_ptr = const int16_t*;    //!< Pointer to @c const int16_t
    using const_int32_ptr = const int32_t*;    //!< Pointer to @c const int32_t
    using const_int64_ptr = const int64_t*;    //!< Pointer to @c const int64_t
    using const_uint8_ptr = const uint8_t*;    //!< Pointer to @c const uint8_t
    using const_uint16_ptr = const uint16_t*;  //!< Pointer to @c const uint16_t
    using const_uint32_ptr = const uint32_t*;  //!< Pointer to @c const uint32_t
    using const_uint64_ptr = const uint64_t*;  //!< Pointer to @c const uint64_t

    //!
    //! Constant meaning "no size", "not found" or "do not resize".
    //! An alternative value for the standard @c std::string::npos value.
    //! Required on Windows to avoid linking issue.
    //!
#if defined(TS_WINDOWS)
    constexpr size_t NPOS = size_t(-1);
#else
    constexpr size_t NPOS = std::string::npos;
#endif

    //!
    //! Enumeration type used to indicate if the data referenced by a pointer shall be copied or shared.
    //!
    enum class ShareMode {
        COPY,  //!< Data shall be copied.
        SHARE  //!< Data shall be shared.
    };

    //!
    //! Tristate boolean.
    //! More generally:
    //! - Zero means false.
    //! - Any positive value means true.
    //! - Any negative value means "maybe" or "dont't know".
    //!
    enum class Tristate {
        Maybe = -1,  //! Undefined value (and more generally all negative values).
        False =  0,  //! Built-in false.
        True  =  1,  //! True value (and more generally all positive values).
    };

    //!
    //! Normalize any integer value in the range of a Tristate value.
    //! @tparam INT An integer type.
    //! @param [in] i The integer value.
    //! @return The corresponding Tristate value.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    Tristate ToTristate(INT i) { return Tristate(std::max<INT>(-1, std::min<INT>(1, i))); }
}


//----------------------------------------------------------------------------
// Optional value, extending C++17 mechanisms.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Set a default value in a std::optional object, if there is none.
    //! @tparam T The type of the optional object.
    //! @tparam U The type of the default value to set.
    //! @param [in,out] opt The optinal object to set.
    //! @param [in] value The value to set in @a obj if it is not initialized.
    //!
    template <class T, class U>
    inline void set_default(std::optional<T>& opt, const U& value)
    {
        if (!opt) {
            opt = value;
        }
    }
}


//----------------------------------------------------------------------------
// File systems, extending C++11 mechanisms.
//----------------------------------------------------------------------------

//!
//! Namespace @c fs is a shortcut for @c std::filesystem.
//!
namespace fs = std::filesystem;

namespace ts {
    //!
    //! This error code is returned by some functions in std::filesystem.
    //!
    constexpr std::uintmax_t FS_ERROR = static_cast<std::uintmax_t>(-1);
}


//----------------------------------------------------------------------------
// Time and duration, extending C++11 and C++17 mechanisms.
//----------------------------------------------------------------------------

//!
//! Namespace @c cn is a shortcut for @c std::chrono.
//!
namespace cn = std::chrono;

namespace ts {
    // Make sure that standard chrono literals such as 10ms or 3s can be used inside namespace ts.
    using namespace std::chrono_literals;

    //!
    //! A derivative of std::chrono::duration for deciseconds (1/10 of a second).
    //! @see std::chrono::milliseconds
    //! @see std::chrono::seconds
    //!
    using deciseconds = cn::duration<cn::milliseconds::rep, std::deci>;

    //!
    //! Definition of a monotonic time.
    //!
    using monotonic_time = cn::time_point<cn::steady_clock>;
}

// Define missing std::chrono::duration types in C++17.
//! @cond nodoxygen
#if !defined(TS_CXX20)
namespace std::chrono {
    using days   = duration<hours::rep, ratio_multiply<ratio<24>, hours::period>>;
    using weeks  = duration<hours::rep, ratio_multiply<ratio<7>, days::period>>;
    using years  = duration<hours::rep, ratio_multiply<ratio<146097, 400>, days::period>>;
    using months = duration<hours::rep, ratio_divide<years::period, ratio<12>>>;
}
#endif
//! @endcond


//----------------------------------------------------------------------------
// Locking and thread-safety, extending C++11 mechanisms.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! A null_mutex class which can be used to replace @c std::mutex or @c std::recursive_mutex.
    //! Used to instantiate synchronized template classes in a mono-thread environment.
    //!
    class TSDUCKDLL null_mutex
    {
    public:
        //! Lock the null_mutex (does nothing).
        void lock() noexcept {}
        //! Unlock the null_mutex (does nothing).
        void unlock() noexcept {}
        //! Successfully try to lock the null_mutex.
        //! @return Always true.
        bool try_lock() noexcept { return true; }
    };

    //!
    //! Thread safety property of a class.
    //!
    enum class ThreadSafety {
        Full,  //!< Thread safe, can be used from multiple threads.
        None,  //!< No thread safety, use in one single thread or use explicit synchronization.
    };

    //!
    //! Define the appropriate mutex class for a given level of thread-safety.
    //! @tparam THS Thread-safety level.
    //!
    template<ThreadSafety THS>
    struct ThreadSafetyMutex {
        //! Mutex class for a given level of thread-safety.
        //! The default is std::mutex and a non-thread-safe specialization uses ts::null_mutex.
        using type = std::mutex;
    };

    // Specialization for monothread.
    //! @cond nodoxygen
    template<> struct ThreadSafetyMutex<ThreadSafety::None> {
        using type = ts::null_mutex;
    };
    //! @endcond
}

// Template specialization on null_mutex of standard lock_guard: reduce overhead to nothing.
//! @cond nodoxygen
namespace std {
    template<> class lock_guard<ts::null_mutex>
    {
    public:
        explicit lock_guard(ts::null_mutex&) {}
        lock_guard(ts::null_mutex&, std::adopt_lock_t) {}
    };
}
//! @endcond

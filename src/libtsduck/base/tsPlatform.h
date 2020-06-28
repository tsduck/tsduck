//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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
//!  @section issues Solving various compilation issues
//!
//!  This header file defines some macros which can be used to solve
//!  various C/C++ compilation issues.
//!
//!  @li Explicitly unused variables: See @link TS_UNUSED @endlink.
//!  @li Definitions of C++ constants: See @link TS_NEED_STATIC_CONST_DEFINITIONS @endlink.
//!
//----------------------------------------------------------------------------

#pragma once


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

#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
    #define TS_GCC_VERSION ((10000 * __GNUC__) + (100 * __GNUC_MINOR__) + (__GNUC_PATCHLEVEL__ % 100))
#endif

#if defined(DOXYGEN)
    //!
    //! Defined when the compiler is compliant with C++11.
    //!
    #define TS_CXX11
    //!
    //! Defined when the compiler is compliant with C++14.
    //!
    #define TS_CXX14
    //!
    //! Defined when the compiler is compliant with C++17.
    //!
    #define TS_CXX17
    //!
    //! Defined when the STL @c basic_string is compliant with C++11.
    //!
    //! The class @c basic_string from the C++ STL has more features starting
    //! with C++11. However, GCC does not support them with old versions, even
    //! if --std=c++11 or 14 is specified. The level of language is increased
    //! but not the level of STL.
    //!
    #define TS_CXX11_STRING
#else
    //
    // Standard ways of signaling the language level.
    //
    #if defined(__cplusplus) && __cplusplus >= 201103L && !defined(TS_CXX11)
        #define TS_CXX11 1
    #endif
    #if defined(__cplusplus) && __cplusplus >= 201402L && !defined(TS_CXX14)
        #define TS_CXX14 1
    #endif
    #if defined(__cplusplus) && __cplusplus >= 201703L && !defined(TS_CXX17)
        #define TS_CXX17 1
    #endif
    //
    // Microsoft-specific ways of signaling the language level.
    // With Visual Studio 2017, there are flags such as /std:c++14 to specify the
    // level of language. However, __cplusplus is still set to 199711L. It is unclear
    // if this is a bug or if __cplusplus is not set because the C++11/14/17 standards
    // all not super-completely implemented. The Microsoft-specific symbol _MSVC_LANG
    // is defined to describe a "good-enough" level of standard which is fine for us.
    //
    #if defined(_MSVC_LANG) && _MSVC_LANG >= 201103L && !defined(TS_CXX11)
        #define TS_CXX11 1
    #endif
    #if defined(_MSVC_LANG) && _MSVC_LANG >= 201402L && !defined(TS_CXX14)
        #define TS_CXX14 1
    #endif
    #if defined(_MSVC_LANG) && _MSVC_LANG >= 201403L && !defined(TS_CXX17)
        #define TS_CXX17 1
    #endif
    //
    // Compliance of the STL.
    // To get C++11 strings, we need C++11. But, with GCC, we need at least GCC 5.x,
    // even if it pretends to be C++11. Except when the actual compiler is LLVM which
    // pretends to be compatible with GCC 4.x but supports C++11 string in fact...
    //
    #if defined(TS_CXX11) && (defined(TS_LLVM) || !defined(__GNUC__) || __GNUC__ >= 5) && !defined(TS_CXX11_STRING)
        #define TS_CXX11_STRING 1
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

#elif defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    #if !defined(TS_WINDOWS)
        #define TS_WINDOWS 1
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

#if !defined(TS_UNIX) && (defined(TS_LINUX) || defined(TS_MAC) || defined(TS_AIX) || defined(TS_CYGWIN) || defined(TS_SOLARIS))
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
    //! Defined when the target processor architecture is ARM.
    //!
    #define TS_ARM
    //!
    //! Defined when the target processor architecture is ARM64.
    //!
    #define TS_ARM64
    //!
    //! Defined when the target processor architecture is STxP70.
    //!
    #define TS_STXP70
    //!
    //! Defined when the target processor architecture is Intel IA-64 architecture, also known as Itanium.
    //!
    #define TS_IA64
    //!
    //! Defined when the target processor architecture is 32-bit Power PC.
    //!
    #define TS_POWERPC
    //!
    //! Defined when the target processor architecture is 64-bit Power PC.
    //!
    #define TS_POWERPC64
    //!
    //! Defined when the target processor architecture is Digital Alpha architecture.
    //!
    #define TS_ALPHA
    //!
    //! Defined when the target processor architecture is Sun SPARC architecture.
    //!
    #define TS_SPARC
    //!
    //! Defined when the target processor architecture is MIPS architecture.
    //!
    #define TS_MIPS

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
#elif defined(__arm__)
    #if !defined(TS_ARM)
        #define TS_ARM 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__aarch64__)
    #if !defined(TS_ARM64)
        #define TS_ARM64 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
    #endif
#elif defined(__stxp70__) || defined(__STxP70__)
    #if !defined(TS_STXP70)
        #define TS_STXP70 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#elif defined(__alpha__)
    #if !defined(TS_ALPHA)
        #define TS_ALPHA 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 64
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
#elif defined(__mips__)
    #if !defined(TS_MIPS)
        #define TS_MIPS 1
    #endif
    #if !defined(TS_ADDRESS_BITS)
        #define TS_ADDRESS_BITS 32
    #endif
#else
    #error "New unknown processor, please update tsPlatform.h"
#endif

// Untested platform warnings.
// Should really use "#warning" instead of "#error" but #warning is a
// GCC extension while #error is standard.

#if !defined(TS_LINUX) && !defined(TS_WINDOWS) && !defined(TS_MAC)
    #error "TSDuck has been tested on Linux, macOS and Windows only, review this code"
#endif

#if !defined(TS_GCC) && !defined(TS_MSC)
    #error "TSDuck has been tested with GCC and MSVC compilers only, review this code"
#endif

// Byte order

#if (defined(TS_I386) || defined(TS_X86_64) || defined(TS_IA64) || defined(TS_ALPHA)) && !defined(TS_LITTLE_ENDIAN)
    #define TS_LITTLE_ENDIAN 1
#elif (defined(TS_SPARC) || defined(TS_POWERPC) || defined(TS_POWERPC64)) && !defined(TS_BIG_ENDIAN)
    #define TS_BIG_ENDIAN 1
#endif

#if defined(TS_ARM) || defined(TS_ARM64) || defined(TS_MIPS)
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define TS_LITTLE_ENDIAN 1
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define TS_BIG_ENDIAN 1
    #else
        #error "ARM endianness not defined"
    #endif
#endif

#if !defined(TS_LITTLE_ENDIAN) && !defined(TS_BIG_ENDIAN)
    #error "unknow endian, please update this header file"
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

//!
//! This macro should preceed an intentional fallthrough in a switch expression.
//!
#if defined(TS_LLVM)
    #define TS_FALLTHROUGH [[clang::fallthrough]];
#elif defined(__GNUC__) && (__GNUC__ >= 7)
    #define TS_FALLTHROUGH __attribute__((fallthrough));
#else
    #define TS_FALLTHROUGH /* fall through */
#endif

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

// Use a two-step macro to allow stringification of parameters.
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


//----------------------------------------------------------------------------
// System-specific settings
//----------------------------------------------------------------------------

// Windows specific settings

#if defined(TS_WINDOWS) && !defined(DOXYGEN)
    #if !defined(WINVER)
        #define WINVER 0x0601            // Allow use of features specific to Windows 7 or later.
    #endif
    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0601      // Allow use of features specific to Windows 7 or later.
    #endif
    #if defined(UNICODE)
        #undef UNICODE                   // No unicode in TSDuck, use single byte char
    #endif
    #if !defined(_CRT_SECURE_NO_DEPRECATE)
        #define _CRT_SECURE_NO_DEPRECATE 1
    #endif
    #if !defined(_CRT_NONSTDC_NO_DEPRECATE)
        #define _CRT_NONSTDC_NO_DEPRECATE 1
    #endif
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1        // Exclude rarely-used stuff from Windows headers
    #endif
#endif

// Large file system (LFS) support on Linux.

#if defined(TS_LINUX) && !defined(DOXYGEN)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(reserved-id-macro)
    #if !defined(_LARGEFILE_SOURCE)
        #define _LARGEFILE_SOURCE    1
    #endif
    #if !defined(_LARGEFILE64_SOURCE)
        #define _LARGEFILE64_SOURCE  1
    #endif
    #if !defined(_FILE_OFFSET_BITS)
        #define _FILE_OFFSET_BITS   64
    #endif
    TS_POP_WARNING()
#endif

// Enforce assertions, even in optimized mode, if TS_KEEP_ASSERTIONS is defined.

#if defined(NDEBUG) && defined(TS_KEEP_ASSERTIONS)
    #undef NDEBUG
#endif

// Disable some warnings, application-wide, for various compilers.

TS_GCC_NOWARNING(unused-parameter)          // Unused parameters are frequent with overrides.
#if defined(TS_ARM)
    TS_GCC_NOWARNING(psabi)
#endif

TS_LLVM_NOWARNING(deprecated)                     // Most of these messages are informational only.
TS_LLVM_NOWARNING(unused-parameter)               // Unused parameters are frequent with overrides.
TS_LLVM_NOWARNING(global-constructors)            // Do not warn about static/global objects being constructed.
TS_LLVM_NOWARNING(exit-time-destructors)          // Do not warn about static/global objects being destructed.
TS_LLVM_NOWARNING(inconsistent-missing-destructor-override) // Requesting "override" for destructors is idiotic, simply.
TS_LLVM_NOWARNING(covered-switch-default)         // Allow "default" in "switch" after all enum values to catch invalid binary values.
TS_LLVM_NOWARNING(sign-conversion)                // Too many occurences since pointer arithmetics is signed, opposite from size_t.
TS_LLVM_NOWARNING(padded)                         // Do not care if padding is required between class fields.
TS_LLVM_NOWARNING(reserved-id-macro)              // We sometimes use underscores at the beginning of identifiers.
TS_LLVM_NOWARNING(c++98-compat-pedantic)          // Require C++11, no need for C++98 compatibility.
TS_LLVM_NOWARNING(c++2a-compat)                   // Compatibility with C++2a is not yet a concern.
TS_LLVM_NOWARNING(documentation-unknown-command)  // Some valid doxygen directives are unknown to clang.

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
TS_MSC_NOWARNING(5039)  // pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.

// The following ones should really be informational instead of warning:
TS_MSC_NOWARNING(4371)  // layout of class may have changed from a previous version of the compiler due to better packing of member 'xxxx'
TS_MSC_NOWARNING(4514)  // unreferenced inline function has been removed
TS_MSC_NOWARNING(4571)  // catch (...) semantics changed since Visual C++ 7.1; structured exceptions(SEH) are no longer caught
TS_MSC_NOWARNING(4710)  // 'xxx' : function not inlined
TS_MSC_NOWARNING(4711)  // function 'xxx' selected for automatic inline expansion
TS_MSC_NOWARNING(5045)  // Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified

// System headers.
// Before including system headers, we must temporarily suspend some compilation warnings.
// This is especially true for Windows which trigger tons of warnings.
// The normal warning reporting is restored after inclusion.

// [BUG.1] This one is nasty and is a bug in winioctl.h, already reported, never fixed, as usual with MSVC...
// It must be set before pushing warnings.
// tsPlatform.h(840, 1) : error C2220 : warning treated as error - no 'object' file generated
// tsPlatform.h(840, 1) : warning C5031 : #pragma warning(pop) : likely mismatch, popping warning state pushed in different file
// winioctl.h(161, 17) : message:  #pragma warning(push)
// tsPlatform.h(719, 1) : warning C5032 : detected #pragma warning(push) with no corresponding #pragma warning(pop)
// different warnings for older versions of MSVC:  C4193:  #pragma warning(pop): no matching '#pragma warning(push)'
TS_MSC_NOWARNING(5031)
TS_MSC_NOWARNING(5032)
TS_MSC_NOWARNING(4193)

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(reserved-id-macro)
TS_MSC_NOWARNING(4263)
TS_MSC_NOWARNING(4264)
TS_MSC_NOWARNING(4668)
TS_MSC_NOWARNING(4774)
TS_MSC_NOWARNING(5026)
TS_MSC_NOWARNING(5027)
TS_MSC_NOWARNING(5204)

#if defined(TS_WINDOWS)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <userenv.h>
#include <memory.h>
#include <io.h>
#include <mmsystem.h>  // Memory management
#include <psapi.h>     // Process API
#include <comutil.h>   // COM utilities
#include <dshow.h>     // DirectShow (aka ActiveMovie)
#include <dshowasf.h>
#include <amstream.h>
#include <videoacc.h>
#include <ks.h>
#include <ksproxy.h>
#include <ksmedia.h>
#include <bdatypes.h>  // BDA (Broadcast Device Architecture)
#include <bdamedia.h>
#include <bdaiface.h>
#include <bdatif.h>
#include <dsattrib.h>
#include <dvbsiparser.h>
#include <mpeg2data.h>
#include <vidcap.h>
#include <Wincrypt.h>  // Cryptographic services
#include <WinInet.h>
#include <Shellapi.h>

#else

#include <cerrno>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <dlfcn.h>
#include <pwd.h>

#endif

#if defined(TS_LINUX)
#include <limits.h>
#include <sys/mman.h>
#include <byteswap.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/version.h>
#endif

#if defined(TS_MAC)
#include <sys/mman.h>
#include <libproc.h>
#endif

#if !defined(TS_NO_PCSC) // PC/SC support not inhibited by user.
#include <winscard.h>
#if defined(TS_LINUX)
#include <PCSC/reader.h>
#endif
#endif

#include <string>
#include <vector>
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <bitset>
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
#include <typeinfo>

#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstddef>     // size_t
#include <fcntl.h>

TS_POP_WARNING()

// See [BUG.1] above. Try to recover from this sh... bug in winioctl.h
#if defined(TS_MSC)
    #pragma warning(pop)  // one more to compensate for missing one in winioctl.h
#endif

// Required link libraries under Windows.

#if defined(TS_WINDOWS) && defined(TS_MSC)
#pragma comment(lib, "userenv.lib")   // GetUserProfileDirectory
#pragma comment(lib, "psapi.lib")     // GetProcessMemoryInfo
#pragma comment(lib, "winmm.lib")     // timeBeginPeriod
#pragma comment(lib, "quartz.lib")    // DirectShow, DirectX
#pragma comment(lib, "ws2_32.lib")    // Winsock 2
#pragma comment(lib, "winscard.lib")  // PC/SC
#if defined(DEBUG)
#pragma comment(lib, "comsuppwd.lib") // COM utilities
#else
#pragma comment(lib, "comsuppw.lib")
#endif
#endif

// Some standard Windows headers have the very-very bad idea to define common
// words as macros. Also, common function names, used by TSDuck, are defined
// as macros, breaking C++ visibility rules.

#if defined(min)
    #undef min
#endif

#if defined(max)
    #undef max
#endif

#if defined(TRUE)
    #undef TRUE
#endif

#if defined(FALSE)
    #undef FALSE
#endif

#if defined(MAYBE)
    #undef MAYBE
#endif

#if defined(IGNORE)
    #undef IGNORE
#endif

#if defined(CHECK)
    #undef CHECK
#endif

#if defined(COMPUTE)
    #undef COMPUTE
#endif

#if defined(INFO)
    #undef INFO
#endif

#if defined(ERROR)
    #undef ERROR
#endif

#if defined(Yield)
    #undef Yield
#endif

#if defined(CreateDirectory)
    #undef CreateDirectory
#endif

#if defined(DeleteFile)
    #undef DeleteFile
#endif

#if defined(ALTERNATE)
    #undef ALTERNATE
#endif

// For platforms not supporting large files:

#if !defined(TS_WINDOWS) && !defined(O_LARGEFILE) && !defined(DOXYGEN)
    #define O_LARGEFILE 0
#endif

// Identify Linux DVB API version in one value

#if defined(TS_LINUX) || defined(DOXYGEN)
    //!
    //! @hideinitializer
    //! On Linux systems, identify the Linux DVB API version in one value.
    //! Example: TS_DVB_API_VERSION is 503 for DVB API version 5.3.
    //!
    #define TS_DVB_API_VERSION ((DVB_API_VERSION * 100) + DVB_API_VERSION_MINOR)
#endif

// On macOS, sigaction(2) uses the flag named SA_RESETHAND instead of SA_ONESHOT.

#if defined(TS_MAC) && !defined(SA_ONESHOT) && !defined(DOXYGEN)
    #define SA_ONESHOT SA_RESETHAND
#endif


//----------------------------------------------------------------------------
// Basic preprocessing features
//----------------------------------------------------------------------------

//!
//! Attribute for explicitly unused variables.
//!
//! It is sometimes required to declare unused variables. This may be temporary,
//! before completing the code and using the variable later, or for any other
//! reason. When the compiler is used in "paranoid" warning mode, declaring
//! unused variables may trigger a warning or an error.
//!
//! When you know that you need to declare an unused variable, the special macro
//! @c TS_UNUSED shall be used as an @e attribute of the variable. Using this
//! attribute, the compiler will no longer complain that the variable is unused.
//!
//! Example:
//! @code
//! TS_UNUSED int i;
//! @endcode
//!
#if defined(DOXYGEN)
    #define TS_UNUSED platform_specific
#elif defined(TS_GCC)
    #define TS_UNUSED __attribute__ ((unused))
#elif defined(TS_MSC)
    // With MS compiler, there is no such attribute. The unused warnings must be disabled exactly once.
    // warning C4189: 'xxx' : local variable is initialized but not referenced
    #define TS_UNUSED __pragma(warning(suppress:4189))
#else
    #error "New unknown compiler, please update TS_UNUSED in tsPlatform.h"
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
//! On Windows, this attribute exports a symbol out of a DLL
//!
#if defined(TS_WINDOWS)
    #define TS_DLL_EXPORT __declspec(dllexport)
#else
    #define TS_DLL_EXPORT
#endif

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
//! @hideinitializer
//! Compilation of definitions of C++ constants.
//!
//! It is a well-known pattern in C++ to define constants in header files as
//! <code>static const</code>. Example:
//! @code
//! static const int FOO = 1;
//! @endcode
//! According to Stroustrup 10.4.6.2, this is only a declaration, which needs
//! a definition somewhere else, without initialization. Example:
//! @code
//! const int classname::FOO;
//! @endcode
//!
//! This is a really stupid oddity of C++ which is handled differently
//! by compilers:
//!
//! @li GCC, with optimization: The declaration alone is fine.
//!     The definition is accepted.
//! @li GCC, without optimization: Sometimes, depending on the way the
//!     constants are used, the definition is required. Without definition,
//!     the linker complains about an undefined symbol.
//! @li Microsoft C: The definition must not be used. Otherwise, the linker
//!     complains about a multiply defined symbol.
//!
//! As a consequence, there are situations where the definition is
//! required and situations where the definition is forbidden.
//! The presence of the definition shall be conditioned to the macro
//! @c TS_NEED_STATIC_CONST_DEFINITIONS.
//!
//! Example: In tsFoo.h, the constant is declared as:
//! @code
//! namespace ts {
//!     class Foo
//!     {
//!     public:
//!         static const size_t MAX_ENTRIES = 10;
//!         ...
//! @endcode
//!
//! In tsFoo.cpp, the definition of the constant shall be conditionally compiled
//! since all compilers do not behave identically:
//! @code
//! #if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
//! const size_t ts::Foo::MAX_ENTRIES;
//! #endif
//! @endcode
//!
#if defined(TS_GCC) || defined(DOXYGEN)
    #define TS_NEED_STATIC_CONST_DEFINITIONS 1
#endif

//!
//! A macro to disable object copy in the definition of a class.
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
//! A macro to disable default constructor and object copy in the definition of a class.
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


//----------------------------------------------------------------------------
// Properties of some integer types.
//----------------------------------------------------------------------------

//!
//! Defined is @c size_t is an exact overload of some predefined @c uintXX_t.
//! In that case, it is not possible to declare distinct overloads of a function
//! with @c size_t and @c uint32_t or @c uint64_t for instance.
//!
#if defined(DOXYGEN)
    #define TS_SIZE_T_IS_STDINT
#elif (defined(TS_GCC_ONLY) || defined(TS_MSC)) && !defined(TS_SIZE_T_IS_STDINT)
    #define TS_SIZE_T_IS_STDINT 1
#endif


//----------------------------------------------------------------------------
// Definition of common integer literals.
//----------------------------------------------------------------------------

#if defined(DOXYGEN)

    //!
    //! Portable definition of a 64-bit signed literal.
    //!
    //! The C/C++ languages define the syntax for integer literals.
    //! An integer literal is always @e typed.
    //! Without suffix such as in @c 0, the literal has type @c int.
    //! With an @c L suffix, such as in @c 0L, the literal has type @c long.
    //! But there is no standard suffix or syntax for 64-bit literals;
    //! different compilers have different syntaxes.
    //!
    //! This macro is a portable way to write 64-bit signed literals.
    //!
    //! Example:
    //! @code
    //! const int64_t aBigOne = TS_CONST64(0x7FFFFFFFFFFFFFFF);
    //! @endcode
    //!
    #define TS_CONST64(n)
    //!
    //! Portable definition of a 64-bit unsigned literal.
    //!
    //! The C/C++ languages define the syntax for integer literals.
    //! An integer literal is always @e typed.
    //! Without suffix such as in @c 0, the literal has type @c int.
    //! With an @c L suffix, such as in @c 0L, the literal has type @c long.
    //! But there is no standard suffix or syntax for 64-bit literals;
    //! different compilers have different syntaxes.
    //!
    //! This macro is a portable way to write 64-bit unsigned literals.
    //!
    //! Example:
    //! @code
    //! const uint64_t aBigOne = TS_UCONST64(0xFFFFFFFFFFFFFFFF);
    //! @endcode
    //!
    #define TS_UCONST64(n)

#elif defined(TS_MSC)
    #define TS_CONST64(n)  n##i64
    #define TS_UCONST64(n) n##ui64
#else
    #define TS_CONST64(n)  (int64_t(n##LL))
    #define TS_UCONST64(n) (uint64_t(n##ULL))
#endif


//----------------------------------------------------------------------------
// Some common pointer types, typically for casting.
//----------------------------------------------------------------------------

//!
//! TSDuck namespace, containing all TSDuck classes and functions.
//!
namespace ts {
    typedef char*     char_ptr;    //!< Pointer to @c char
    typedef int8_t*   int8_ptr;    //!< Pointer to @c int8_t
    typedef int16_t*  int16_ptr;   //!< Pointer to @c int16_t
    typedef int32_t*  int32_ptr;   //!< Pointer to @c int32_t
    typedef int64_t*  int64_ptr;   //!< Pointer to @c int64_t
    typedef uint8_t*  uint8_ptr;   //!< Pointer to @c uint8_t
    typedef uint16_t* uint16_ptr;  //!< Pointer to @c uint16_t
    typedef uint32_t* uint32_ptr;  //!< Pointer to @c uint32_t
    typedef uint64_t* uint64_ptr;  //!< Pointer to @c uint64_t
    typedef const char*     const_char_ptr;    //!< Pointer to @c const char
    typedef const int8_t*   const_int8_ptr;    //!< Pointer to @c const int8_t
    typedef const int16_t*  const_int16_ptr;   //!< Pointer to @c const int16_t
    typedef const int32_t*  const_int32_ptr;   //!< Pointer to @c const int32_t
    typedef const int64_t*  const_int64_ptr;   //!< Pointer to @c const int64_t
    typedef const uint8_t*  const_uint8_ptr;   //!< Pointer to @c const uint8_t
    typedef const uint16_t* const_uint16_ptr;  //!< Pointer to @c const uint16_t
    typedef const uint32_t* const_uint32_ptr;  //!< Pointer to @c const uint32_t
    typedef const uint64_t* const_uint64_ptr;  //!< Pointer to @c const uint64_t
}


//----------------------------------------------------------------------------
// Byte swapping in integer values.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Perform a sign extension on 24 bit integers.
    //!
    //! @param [in] x A 32-bit integer containing a signed 24-bit value to extend.
    //! @return A 32-bit signed integer containing the signed 24-bit value with proper sign extension on 32-bits.
    //!
    TSDUCKDLL inline int32_t SignExtend24(int32_t x)
    {
        return (x & 0x00800000) == 0 ? (x & 0x00FFFFFF) : int32_t(uint32_t(x) | 0xFF000000);
    }

    //!
    //! Perform a sign extension on 40 bit integers.
    //!
    //! @param [in] x A 64-bit integer containing a signed 40-bit value to extend.
    //! @return A 64-bit signed integer containing the signed 40-bit value with proper sign extension on 64-bits.
    //!
    TSDUCKDLL inline int64_t SignExtend40(int64_t x)
    {
        return (x & TS_UCONST64(0x0000008000000000)) == 0 ? (x & TS_UCONST64(0x000000FFFFFFFFFF)) : int64_t(uint64_t(x) | TS_UCONST64(0xFFFFFF0000000000));
    }

    //!
    //! Perform a sign extension on 48 bit integers.
    //!
    //! @param [in] x A 64-bit integer containing a signed 48-bit value to extend.
    //! @return A 64-bit signed integer containing the signed 48-bit value with proper sign extension on 64-bits.
    //!
    TSDUCKDLL inline int64_t SignExtend48(int64_t x)
    {
        return (x & TS_UCONST64(0x0000800000000000)) == 0 ? (x & TS_UCONST64(0x0000FFFFFFFFFFFF)) : int64_t(uint64_t(x) | TS_UCONST64(0xFFFF000000000000));
    }

    //!
    //! Inlined function performing byte swap on 16-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 16-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint16_t ByteSwap16(uint16_t x)
    {
    #if defined(TS_LINUX)
        return bswap_16(x);
    #elif defined(TS_MSC)
        return _byteswap_ushort(x);
    #else
        return uint16_t((x << 8) | (x >> 8));
    #endif
    }

    //!
    //! Inlined function performing byte swap on 24-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to swap.
    //! @return The value of @a x where the three least significant bytes were swapped.
    //!
    TSDUCKDLL inline uint32_t ByteSwap24(uint32_t x)
    {
        return ((x << 16) & 0x00FF0000) | (x & 0x0000FF00) | ((x >> 16) & 0x000000FF);
    }

    //!
    //! Inlined function performing byte swap on 32-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 32-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint32_t ByteSwap32(uint32_t x)
    {
    #if defined(TS_LINUX)
        return bswap_32(x);
    #elif defined(TS_MSC)
        return _byteswap_ulong(x);
    #else
        return (x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24);
    #endif
    }

    //!
    //! Inlined function performing byte swap on 64-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 64-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint64_t ByteSwap64(uint64_t x)
    {
    #if defined(TS_LINUX)
        return bswap_64(x);
    #elif defined(TS_MSC)
        return _byteswap_uint64(x);
    #else
        return
            ((x << 56)) |
            ((x << 40) & TS_UCONST64(0x00FF000000000000)) |
            ((x << 24) & TS_UCONST64(0x0000FF0000000000)) |
            ((x <<  8) & TS_UCONST64(0x000000FF00000000)) |
            ((x >>  8) & TS_UCONST64(0x00000000FF000000)) |
            ((x >> 24) & TS_UCONST64(0x0000000000FF0000)) |
            ((x >> 40) & TS_UCONST64(0x000000000000FF00)) |
            ((x >> 56));
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16BE(uint16_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap16(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16(uint16_t x)
    {
        return CondByteSwap16BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24BE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap24(x);
    #else
        return x & 0x00FFFFFF;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24(uint32_t x)
    {
        return CondByteSwap24BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32BE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap32(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32(uint32_t x)
    {
        return CondByteSwap32BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64BE(uint64_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap64(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64(uint64_t x)
    {
        return CondByteSwap64BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16LE(uint16_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap16(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24LE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x & 0x00FFFFFF;
    #else
        return ByteSwap24(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32LE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap32(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64LE(uint64_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap64(x);
    #endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwapBE(INT x)
    {
#if defined(TS_BIG_ENDIAN)
        return x;
#else
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: return 0;
        }
#endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwapLE(INT x)
    {
#if defined(TS_BIG_ENDIAN)
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: return 0;
        }
#else
        return x;
#endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwap(INT x)
    {
        return CondByteSwapBE<INT>(x);
    }

    // Some specializations, for performance

#if !defined(DOXYGEN)
    template<> TSDUCKDLL inline uint8_t CondByteSwap   (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwap   (int8_t  x) {return x;}
    template<> TSDUCKDLL inline uint8_t CondByteSwapBE (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwapBE (int8_t  x) {return x;}
    template<> TSDUCKDLL inline uint8_t CondByteSwapLE (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwapLE (int8_t  x) {return x;}
#endif
}


//----------------------------------------------------------------------------
// Rotate functions.
// ROL = rotate left, ROR = rotate right
// ROLc/RORc = rotate with constant value for index (optimized when asm).
// Note that, in debug mode, ROLc/RORc revert to ROL/ROR since the routines
// are not inlined and, thus, constant constraint cannot be checked.
//----------------------------------------------------------------------------

namespace ts {

#if defined(DOXYGEN)
    //!
    //! Inlined function performing 32-bit left-rotate.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (left-rotate) or negative (right-rotate).
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i) {return XX;}

    //!
    //! Inlined function performing 32-bit left-rotate with a constant value in the range 0..31 for index.
    //!
    //! Using @c ROLc instead of @c ROL when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROLc reverts to @c ROL since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..31.
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i) {return XX;}

    //!
    //! Inlined function performing 32-bit right-rotate.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i) {return XX;}

    //!
    //! Inlined function performing 32-bit right-rotate with a constant value in the range 0..31 for index.
    //!
    //! Using @c RORc instead of @c ROR when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note 1: In debug mode, @c RORc reverts to @c ROR since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! Note 2: With the LLVM compiler, @c RORc reverts to @c ROR since the compiled generates
    //! an error and does not recognize the operand as a constant.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..31.
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i) {return XX;}

#elif defined(TS_MSC)
#pragma intrinsic(_lrotr,_lrotl)

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i) {return _lrotl(word, i);}
    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i) {return _lrotr(word, i);}

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i) {return _lrotl(word, i);}
    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i) {return _lrotr(word, i);}

#elif defined(TS_GCC) && (defined(TS_I386) || defined(TS_X86_64))

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        asm ("roll %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        asm ("rorl %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
#if defined(DEBUG) || defined(TS_LLVM)
        return ROL (word, i);
#else
        asm ("roll %2,%0"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
#if defined(DEBUG) || defined(TS_LLVM)
        return ROR (word, i);
#else
        asm ("rorl %2,%0"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

#elif defined(TS_POWERPC)

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        asm ("rotlw %0,%0,%2"
             :"=r" (word)
             :"0" (word),"r" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        asm ("rotlw %0,%0,%2"
             :"=r" (word)
             :"0" (word),"r" (32-i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
#if defined(DEBUG)
        return ROL (word, i);
#else
        asm ("rotlwi %0,%0,%2"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
#if defined(DEBUG)
        return ROR (word, i);
#else
        asm ("rotrwi %0,%0,%2"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

#else

    // Rotates the hard way

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >>(32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        return (((word&0xFFFFFFFFUL) >>(i&31)) | (word << (32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >>(32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
        return (((word&0xFFFFFFFFUL) >>(i&31)) | (word << (32-(i&31)))) & 0xFFFFFFFFUL;
    }

#endif

#if defined(DOXYGEN)
    //!
    //! Inlined function performing 64-bit left-rotate.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (left-rotate) or negative (right-rotate).
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i) {return XX;}

    //!
    //! Inlined function performing 64-bit left-rotate with a constant value in the range 0..63 for index.
    //!
    //! Using @c ROL64c instead of @c ROL64 when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROL64c reverts to @c ROL64 since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..63.
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i) {return XX;}

    //!
    //! Inlined function performing 64-bit right-rotate.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i) {return XX;}

    //!
    //! Inlined function performing 64-bit right-rotate with a constant value in the range 0..63 for index.
    //!
    //! Using @c ROR64c instead of @c ROR64 when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROR64c reverts to @c ROR64 since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..63.
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i) {return XX;}

#elif defined(TS_GCC) && defined(TS_X86_64)

    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i)
    {
        asm ("rolq %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i)
    {
        asm ("rorq %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i)
    {
#if defined(DEBUG) || defined(TS_LLVM)
        return ROL64(word, i);
#else
        asm ("rolq %2,%0"
             :"=r" (word)
             :"0" (word),"J" (i));
        return word;
#endif
    }

    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i)
    {
#if defined(DEBUG) || defined(TS_LLVM)
        return ROR64(word, i);
#else
        asm ("rorq %2,%0"
             :"=r" (word)
             :"0" (word),"J" (i));
        return word;
#endif
    }

#else

    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i)
    {
        return (word << (i&63)) | ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >>(64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i)
    {
        return ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >>(i&63)) | (word << (64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i)
    {
        return (word << (i&63)) | ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >>(64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i)
    {
        return ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >>(i&63)) | (word << (64-(i&63)));
    }

#endif
}


//----------------------------------------------------------------------------
// Cross-platforms portable definitions for memory barrier.
//----------------------------------------------------------------------------

#if defined(TS_MSC)
    #pragma intrinsic(_ReadWriteBarrier)
#endif

#if defined(DOXYGEN) /* documentation only */
    //!
    //! To be defined to implement memory barrier as a no-operation.
    //!
    //! This symbol shall be defined by the developer on the command line
    //! to ensure that no specific memory barrier instruction is generated.
    //!
    //! This can be useful in some environments (for instance using valgrind
    //! on the ARM architecture) when the memory barrier causes some trouble.
    //!
    //! Note that not using memory barrier instructions can cause some extremely
    //! rare race conditions.
    //!
    #define TS_NO_MEMORY_BARRIER
#endif

namespace ts {

    //!
    //! Inlined C function performing a CPU/compiler dependent memory barrier.
    //!
    TSDUCKDLL inline void MemoryBarrier(void)
    {
#if defined(TS_NO_MEMORY_BARRIER)

        // Nothing to do

#elif defined(TS_GCC) && (defined(TS_I386) || defined(TS_X86_64))

        // "mfence" is SSE2, not supported on all x86 cpus but supported on all x86_64 cpus.
        __asm__ __volatile__ ("mfence" : : : "memory");

#elif defined(TS_GCC) && defined(__ARM_ARCH_5TEJ__)

        // Some flavours of the ARM architecture do not support accessing r15 in user mode.
        // Simply prevent the compiler from rescheduling instructions (not a true "memory barrier" however).
        __asm__ __volatile__ ("" : : :  "memory");

#elif defined(TS_GCC) && defined(TS_ARM)

        // For later reference, not sure this is valid.
        unsigned dest = 0;
        __asm__ __volatile__ ("@MemoryBarrier\n mcr p15,0,%0,c7,c10,5\n" : "=&r"(dest) : :  "memory");

#elif defined(TS_GCC) && defined(TS_ARM64)

        __asm__ __volatile__ ("dmb sy" : : : "memory");

#elif defined(TS_GCC) && defined(TS_MIPS)

       __asm__ __volatile__ ("sync" : : :"memory");

#elif defined(TS_MSC)

        // Prevent the compiler from reordering memory access
        _ReadWriteBarrier();
        // CPU memory barrier
        ::MemoryBarrier();

#else
    #error "MemoryBarrier is not implemented on this platform"
#endif
    }
}


//----------------------------------------------------------------------------
// System error codes
//----------------------------------------------------------------------------

namespace ts {

    //!
    //! Integer type for operating system error codes.
    //!
#if defined(DOXYGEN)
    typedef platform_specific ErrorCode;
#elif defined(TS_WINDOWS)
    typedef ::DWORD ErrorCode;
#else
    typedef int ErrorCode;
#endif

    //!
    //! An @link ErrorCode @endlink value indicating success.
    //!
    //! It is not guaranteed that this value is the @e only success value.
    //! Operating system calls which complete successfully may also return
    //! other values.
    //!
#if defined(DOXYGEN)
    const ErrorCode SYS_SUCCESS = platform_specific;
#elif defined(TS_WINDOWS)
    const ErrorCode SYS_SUCCESS = ERROR_SUCCESS;
#elif defined(TS_UNIX)
    const ErrorCode SYS_SUCCESS = 0;
#else
    #error "Unsupported operating system"
#endif

    //!
    //! An @link ErrorCode @endlink value indicating a generic data error.
    //!
    //! This value can be used to initialize an error code to some generic
    //! error code indicating that a data is not yet available or an
    //! operation is not yet performed.
    //!
#if defined(DOXYGEN)
    const ErrorCode SYS_DATA_ERROR = platform_specific;
#elif defined(TS_WINDOWS)
    const ErrorCode SYS_DATA_ERROR = ERROR_INVALID_DATA;
#elif defined(TS_UNIX)
    const ErrorCode SYS_DATA_ERROR = EINVAL;
#else
    #error "Unsupported operating system"
#endif

    //!
    //! Get the error code of the last operating system call.
    //!
    //! The validity of the returned value may depends on specific conditions.
    //!
    //! @return The error code of the last operating system call.
    //!
    TSDUCKDLL inline ErrorCode LastErrorCode()
    {
#if defined(TS_WINDOWS)
        return ::GetLastError();
#elif defined(TS_UNIX)
        return errno;
#else
        #error "Unsupported operating system"
#endif
    }
}


//----------------------------------------------------------------------------
// Request type for ioctl.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Portable type for ioctl() request parameter.
    //!
#if defined(DOXYGEN)
    typedef platform-dependent ioctl_request_t;
#elif defined(TS_WINDOWS)
    // Second parameter of ::DeviceIoControl().
    typedef ::DWORD ioctl_request_t;
#else
    // Extract the type of the second parameter of ::ioctl().
    // It is "unsigned long" on most Linux systems but "int" on Alpine Linux.
    template<typename T>
    T request_param_type(int (*ioctl_syscall)(int, T, ...));
    typedef decltype(request_param_type(&::ioctl)) ioctl_request_t;
#endif
}


//----------------------------------------------------------------------------
// Socket programming portability macros.
// Most socket types and functions have identical API in UNIX and Windows.
// However, there are some slight incompatibilities which are solved by
// using the following macros.
//----------------------------------------------------------------------------

#if defined(DOXYGEN)

//!
//! Data type for socket descriptors as returned by the socket() system call.
//!
typedef platform_specific TS_SOCKET_T;

//!
//! Value of type TS_SOCKET_T which is returned by the socket() system call in case of failure.
//! Example:
//! @code
//! TS_SOCKET_T sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
//! if (sock == TS_SOCKET_T_INVALID) {
//!     ... error processing ...
//! }
//! @endcode
//!
#define TS_SOCKET_T_INVALID platform_specific

//!
//! Integer data type which receives the length of a struct sockaddr.
//! Example:
//! @code
//! struct sockaddr sock_addr;
//! TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
//! if (getsockname(sock, &sock_addr, &len) != 0) {
//!     ... error processing ...
//! }
//! @endcode
//!
typedef platform_specific TS_SOCKET_SOCKLEN_T;

//!
//! Integer data type for a "signed size" returned from send() or recv() system calls.
//! Example:
//! @code
//! TS_SOCKET_SSIZE_T got = recv(sock, TS_RECVBUF_T(&data), max_size, 0);
//! @endcode
//!
typedef platform_specific TS_SOCKET_SSIZE_T;

//!
//! Integer data type for the Time To Live (TTL) socket option.
//! Example:
//! @code
//! TS_SOCKET_TTL_T ttl = 10;
//! if (setsockopt(sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&ttl), sizeof(ttl)) != 0) {
//!     ... error processing ...
//! }
//! @endcode
//!
typedef platform_specific TS_SOCKET_TTL_T;

//!
//! Integer data type for the TOS socket option.
//!
typedef platform_specific TS_SOCKET_TOS_T;

//!
//! Integer data type for the multicast Time To Live (TTL) socket option.
//! Example:
//! @code
//! TS_SOCKET_MC_TTL_T mttl = 1;
//! if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, TS_SOCKOPT_T(&mttl), sizeof(mttl)) != 0) {
//!     ... error processing ...
//! }
//! @endcode
//!
typedef platform_specific TS_SOCKET_MC_TTL_T;

//!
//! Type conversion macro for the field l_linger in the struct linger socket option.
//! All systems do not use the same type size and this may generate some warnings.
//! Example:
//! @code
//! struct linger lin;
//! lin.l_linger = TS_SOCKET_L_LINGER_T(seconds);
//! @endcode
//!
#define TS_SOCKET_L_LINGER_T(x) platform_specific

//!
//! Integer data type for the IP_PKTINFO socket option.
//! Example:
//! @code
//! TS_SOCKET_PKTINFO_T state = 1;
//! if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, TS_SOCKOPT_T(&state), sizeof(state)) != 0) {
//!     ... error processing ...
//! }
//! @endcode
//!
typedef platform_specific TS_SOCKET_PKTINFO_T;

//!
//! Type conversion macro for the address of a socket option value.
//! The "standard" parameter type is @c void* but some systems use other exotic values.
//! Example:
//! @code
//! TS_SOCKET_TTL_T ttl = 10;
//! if (setsockopt(sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&ttl), sizeof(ttl)) != 0) {
//!     ... error processing ...
//! }
//! @endcode
//!
#define TS_SOCKOPT_T(x) platform_specific

//!
//! Type conversion macro for the address of the data buffer for a recv() system call.
//! The "standard" parameter type is @c void* but some systems use other exotic values.
//! Example:
//! @code
//! TS_SOCKET_SSIZE_T got = recv(sock, TS_RECVBUF_T(&data), max_size, 0);
//! @endcode
//!
#define TS_RECVBUF_T(x) platform_specific

//!
//! Type conversion macro for the address of the data buffer for a send() system call.
//! The "standard" parameter type is @c void* but some systems use other exotic values.
//! Example:
//! @code
//! TS_SOCKET_SSIZE_T gone = send(sock, TS_SENDBUF_T(&data), size, 0);
//! @endcode
//!
#define TS_SENDBUF_T(x) platform_specific

//!
//! Name of the ioctl() system call which applies to socket devices.
//! The "standard" name is @c ioctl but some systems use other exotic names.
//! Note that the ioctl() system call is rarely used on sockets.
//! Most options are accessed through getsockopt() and setsockopt().
//!
#define TS_SOCKET_IOCTL platform_specific

//!
//! Name of the close() system call which applies to socket devices.
//! The "standard" name is @c close but some systems use other exotic names.
//! Example:
//! @code
//! TS_SOCKET_CLOSE(sock);
//! @endcode
//!
#define TS_SOCKET_CLOSE platform_specific

//!
//! Name of the option for the shutdown() system call which means "close on both directions".
//! Example:
//! @code
//! shutdown(sock, TS_SOCKET_SHUT_RDWR);
//! @endcode
//!
#define TS_SOCKET_SHUT_RDWR platform_specific

//!
//! Name of the option for the shutdown() system call which means "close on receive side".
//! Example:
//! @code
//! shutdown(sock, TS_SOCKET_SHUT_RD);
//! @endcode
//!
#define TS_SOCKET_SHUT_RD platform_specific

//!
//! Name of the option for the shutdown() system call which means "close on send side".
//! Example:
//! @code
//! shutdown(sock, TS_SOCKET_SHUT_WR);
//! @endcode
//!
#define TS_SOCKET_SHUT_WR platform_specific

//!
//! System error code value meaning "connection reset by peer".
//!
#define TS_SOCKET_ERR_RESET platform_specific

//!
//! System error code value meaning "peer socket not connected".
//!
#define TS_SOCKET_ERR_NOTCONN platform_specific

#elif defined(TS_WINDOWS)

#define TS_SOCKET_L_LINGER_T(x) (static_cast<u_short>(x))
#define TS_SOCKOPT_T(x)         (reinterpret_cast<const char*>(x))
#define TS_RECVBUF_T(x)         (reinterpret_cast<char*>(x))
#define TS_SENDBUF_T(x)         (reinterpret_cast<const char*>(x))
#define TS_SOCKET_IOCTL         ::ioctlsocket
#define TS_SOCKET_CLOSE         ::closesocket
#define TS_SOCKET_T_INVALID     INVALID_SOCKET
#define TS_SOCKET_SHUT_RDWR     SD_BOTH
#define TS_SOCKET_SHUT_RD       SD_RECEIVE
#define TS_SOCKET_SHUT_WR       SD_SEND
#define TS_SOCKET_ERR_RESET     WSAECONNRESET
#define TS_SOCKET_ERR_NOTCONN   WSAENOTCONN

typedef ::SOCKET TS_SOCKET_T;
typedef int TS_SOCKET_SOCKLEN_T;
typedef int TS_SOCKET_SSIZE_T;
typedef ::DWORD TS_SOCKET_TOS_T;
typedef ::DWORD TS_SOCKET_TTL_T;
typedef ::DWORD TS_SOCKET_MC_TTL_T;
typedef ::DWORD TS_SOCKET_PKTINFO_T;

#elif defined(TS_UNIX)

#define TS_SOCKET_L_LINGER_T(x) (static_cast<int>(x))
#define TS_SOCKOPT_T(x)         (x)
#define TS_RECVBUF_T(x)         (x)
#define TS_SENDBUF_T(x)         (x)
#define TS_SOCKET_IOCTL         ::ioctl
#define TS_SOCKET_CLOSE         ::close
#define TS_SOCKET_T_INVALID     (-1)
#define TS_SOCKET_SHUT_RDWR     SHUT_RDWR
#define TS_SOCKET_SHUT_RD       SHUT_RD
#define TS_SOCKET_SHUT_WR       SHUT_WR
#define TS_SOCKET_ERR_RESET     EPIPE
#define TS_SOCKET_ERR_NOTCONN   ENOTCONN

typedef int TS_SOCKET_T;
typedef ::socklen_t TS_SOCKET_SOCKLEN_T;
typedef ::ssize_t TS_SOCKET_SSIZE_T;
typedef int TS_SOCKET_TOS_T;
typedef int TS_SOCKET_TTL_T;
typedef unsigned char TS_SOCKET_MC_TTL_T;
typedef int TS_SOCKET_PKTINFO_T;

#else
#error "check socket compatibility macros on this platform"
#endif


//----------------------------------------------------------------------------
// Some integer constants.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Constant meaning "no size", "not found" or "do not resize".
    //! An alternative value for the standard @c std::string::npos value.
    //! Required on Windows to avoid linking issue.
    //!
    const size_t NPOS =
#if defined(TS_WINDOWS)
        size_t(-1);
#else
        std::string::npos;
#endif

}


//----------------------------------------------------------------------------
// Time-related definition
//----------------------------------------------------------------------------

namespace ts {

    //!
    //! This integer type is used to represent any sub-quantity of seconds.
    //!
    //! This type is mostly used as parent for all other representations
    //! of sub-quantities of seconds (@link MilliSecond @endlink,
    //! @link NanoSecond @endlink, etc.) Although these types are all
    //! identical, they should be used explicitly for clarity. Thus,
    //! when reading some code, it the variable for a duration has type
    //! MilliSecond, it is clear that it contains a number of milliseconds,
    //! not seconds or microseconds.
    //!
    //! Note that this is a signed type. A number of sub-quantities of seconds
    //! can be negative, indicating a duration backward.
    //!
    typedef int64_t SubSecond;
    //!
    //! This integer type is used to represent a number of seconds.
    //! Should be explicitly used for clarity when a variable contains a number of seconds.
    //!
    typedef SubSecond Second;
    //!
    //! This integer type is used to represent a number of milliseconds.
    //! Should be explicitly used for clarity when a variable contains a number of milliseconds.
    //!
    typedef SubSecond MilliSecond;
    //!
    //! This integer type is used to represent a number of microseconds.
    //! Should be explicitly used for clarity when a variable contains a number of microseconds.
    //!
    typedef SubSecond MicroSecond;
    //!
    //! This integer type is used to represent a number of nanoseconds.
    //! Should be explicitly used for clarity when a variable contains a number of nanoseconds.
    //!
    typedef SubSecond NanoSecond;
    //!
    //! This constant shall be used by convention to express an infinite
    //! number of sub-quantities of seconds.
    //!
    constexpr SubSecond Infinite = TS_CONST64(0x7FFFFFFFFFFFFFFF);
    //!
    //! Number of nanoseconds per second
    //!
    constexpr NanoSecond NanoSecPerSec = 1000000000;
    //!
    //! Number of nanoseconds per millisecond
    //!
    constexpr NanoSecond NanoSecPerMilliSec = 1000000;
    //!
    //! Number of nanoseconds per microsecond
    //!
    constexpr NanoSecond NanoSecPerMicroSec = 1000;
    //!
    //! Number of microseconds per second
    //!
    constexpr MicroSecond MicroSecPerSec = 1000000;
    //!
    //! Number of microseconds per millisecond
    //!
    constexpr MicroSecond MicroSecPerMilliSec = 1000;
    //!
    //! Number of milliseconds per second
    //!
    constexpr MilliSecond MilliSecPerSec = 1000;
    //!
    //! Number of milliseconds per minute
    //!
    constexpr MilliSecond MilliSecPerMin = 1000 * 60;
    //!
    //! Number of milliseconds per hour
    //!
    constexpr MilliSecond MilliSecPerHour = 1000 * 60 * 60;
    //!
    //! Number of milliseconds per day
    //!
    constexpr MilliSecond MilliSecPerDay = 1000 * 60 * 60 * 24;
}


//----------------------------------------------------------------------------
//  General purpose enumeration types
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Enumeration type used to indicate if the data referenced by a pointer shall be copied or shared.
    enum class ShareMode {
        COPY,  //!< Data shall be copied.
        SHARE  //!< Data shall be shared.
    };
}


//----------------------------------------------------------------------------
//  Tristate boolean.
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Tristate boolean.
    //! More generally:
    //! - Zero means false.
    //! - Any positive value means true.
    //! - Any negative value means "maybe" or "dont't know".
    //!
    enum class Tristate {
        MAYBE = -1,  //! Undefined value (and more generally all negative values).
        FALSE =  0,  //! Built-in false.
        TRUE  =  1,  //! True value (and more generally all positive values).
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
// Define a mechanism to identify source code and compilation time in the
// object files.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)
//
// The macro TS_SET_BUILD_MARK defines an internal string data in the
// object file containin a string. The difficulty is to make sure that
// the compiler will not optimize away this data (it is local and unused).
//
#if defined(TS_GCC)
#define TS_SET_BUILD_MARK(s) static __attribute__ ((used)) const char* const _tsBuildMark = (s)
#else
#define TS_SET_BUILD_MARK(s)                                                  \
    namespace {                                                               \
        class _tsBuildMarkClass {                                             \
            TS_NOBUILD_NOCOPY(_tsBuildMarkClass);                             \
        public:                                                               \
            const char* const str;                                            \
            _tsBuildMarkClass(const char* _s): str(_s) {}                     \
        };                                                                    \
        const _tsBuildMarkClass _tsBuildMark(s);                              \
    }
#endif

//
// Define the prefix which is used to locate the build marker string in the object file.
// The first character in the prefix will be used as field separator.
// By default, the TSDuck version string is included in each binary.
// The price to pay is to recompile everything after a git commit since each commit
// updates the globale TSDuck version. To avoid this, define TS_NO_BUILD_VERSION.
//
#define TS_BUILD_MARK_SEPARATOR "|"
#define TS_BUILD_MARK_MARKER "@($%)"
#if defined(TS_NO_BUILD_VERSION)
    #define TS_BUILD_VERSION
#else
    #include "tsVersionString.h"
    #define TS_BUILD_VERSION TS_BUILD_MARK_SEPARATOR TS_VERSION_STRING
#endif
#define TS_BUILD_MARK_PREFIX TS_BUILD_MARK_SEPARATOR TS_BUILD_MARK_MARKER TS_BUILD_MARK_SEPARATOR "tsduck" TS_BUILD_VERSION TS_BUILD_MARK_SEPARATOR

//
// Disable one LLVM warning about non-reproduceability of data and time.
// This sequence should be portable. However, GCC complains about pragmas
// in this context. Probably a GCC bug but we need to handle it.
//
#if defined(TS_LLVM)
    #define TSDUCK_SOURCE_BEGIN  TS_PUSH_WARNING() TS_LLVM_NOWARNING(date-time)
    #define TSDUCK_SOURCE_END    TS_POP_WARNING()
#else
    #define TSDUCK_SOURCE_BEGIN
    #define TSDUCK_SOURCE_END
#endif

#endif // DOXYGEN

//!
//! @hideinitializer
//!
//! This macro inserts a build mark in the source code, identifying it as part or TSDuck.
//!
//! The string is built from fields, separated by @c '|'.
//! The list of fields is:
//! - Marker prefix: @c @($%)
//! - String literal "tsduck"
//! - TSDuck version number.
//! - Compilation date. According to the C11 and C++11 standards, the value of the macro
//!   __DATE__ is a character string literal of the form "Mmm dd yyyy", where the names of
//!   the months are the same as those generated by the asctime function, and the first
//!   character of dd is a space character if the value is less than 10.
//! - Compilation time. According to the C11 and C++11 standards, the value of the macro
//!   __TIME__ is a character string literal of the form "hh:mm:ss" as in the time generated
//!   by the asctime function.
//! - Name of the source file which invokes the macro.
//!
#define TSDUCK_SOURCE \
    TSDUCK_SOURCE_BEGIN \
    TS_SET_BUILD_MARK(TS_BUILD_MARK_PREFIX __DATE__ TS_BUILD_MARK_SEPARATOR __TIME__ TS_BUILD_MARK_SEPARATOR __FILE__ TS_BUILD_MARK_SEPARATOR) \
    TSDUCK_SOURCE_END

//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
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
//!  @file tsPlatform.h
//!
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
//!  @li Compiler: See @link __gcc @endlink, @link __msc @endlink, etc.
//!  @li Operating system: See @link __linux @endlink, @link __windows @endlink, etc.
//!  @li Byte ordering: See @link __little_endian @endlink and @link __big_endian @endlink.
//!  @li Processor architecture: See @link __i386 @endlink, @link __x86_64 @endlink, etc.
//!
//!  @section issues Solving various compilation issues
//!
//!  This header file defines some macros which can be used to solve
//!  various C/C++ compilation issues.
//!
//!  @li Explicitly unused variables: See @link TS_UNUSED @endlink.
//!  @li The null pointer and C++: See @link TS_NULL @endlink.
//!  @li Definitions of C++ constants: See @link TS_NEED_STATIC_CONST_DEFINITIONS @endlink.
//!
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// Unified compiler naming: __gcc, __msc (Microsoft C)
//----------------------------------------------------------------------------

#if defined(DOXYGEN)

    //!
    //! Defined when the compiler is GCC, also known as "GNU C".
    //!
    #define __gcc
    //!
    //! Defined when the compiler is LLVM (clang).
    //! In that case, __gcc is also defined since LLVM is reasonably compatible with GCC.
    //!
    #define __llvm
    //!
    //! Defined when the compiler is Microsoft C/C++, the default compiler
    //! in the Microsoft Visual Studio environment. Also used on command line
    //! and batch file as the @c cl command.
    //!
    #define __msc

#elif defined(__llvm__) || defined(__clang__)
    #if !defined(__llvm)
        #define __llvm 1
    #endif
    #if !defined(__gcc)
        #define __gcc 1
    #endif
#elif defined(__GNUC__)
    #if !defined(__gcc)
        #define __gcc 1
    #endif
#elif defined(_MSC_VER)
    #if !defined(__msc)
        #define __msc 1
    #endif
#else
    #error "New unknown compiler, please update tsPlatform.h"
#endif


//----------------------------------------------------------------------------
// Unified O/S naming: __linux, __windows, etc
//----------------------------------------------------------------------------

#if defined(DOXYGEN)

    //!
    //! Defined when compiled for a Microsoft Windows target platform.
    //!
    #define __windows
    //!
    //! Defined when compiled for any flavor of UNIX target platforms.
    //!
    //! This symbol comes in addition to the specific symbol for the
    //! target platform (@link __linux @endlink, etc.)
    //!
    #define __unix
    //!
    //! Defined when compiled for a Linux target platform.
    //!
    #define __linux
    //!
    //! Defined when compiled for a MacOS target platform.
    //!
    #define __mac
     //!
    //! Defined when compiled for an IBM AIX target platform.
    //!
    #define __aix
    //!
    //! Defined when compiled for a Sun Solaris target platform.
    //!
    #define __solaris
    //!
    //! Defined when compiled for a Cygwin target platform.
    //!
    #define __cygwin

#elif defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    #if !defined(__windows)
        #define __windows 1
    #endif
#elif defined(__gnu_linux__) || defined(__linux) || defined(__linux__) || defined(linux)
    #if !defined(__linux)
        #define __linux 1
    #endif
#elif defined(__APPLE__)
    #if !defined(__mac)
        #define __mac 1
    #endif
#elif defined(_AIX) || defined(__aix)
    #if !defined(__aix)
        #define __aix 1
    #endif
#elif defined(__CYGWIN__) || defined(__cygwin)
    #if !defined(__cygwin)
        #define __cygwin 1
    #endif
#elif defined(__sun) || defined(__solaris)
    #if !defined(__solaris)
        #define __solaris 1
    #endif
#else
    #error "New unknown operating system, please update tsPlatform.h"
#endif

#if !defined(__unix) && (defined(__linux) || defined(__mac) || defined(__aix) || defined(__cygwin) || defined(__solaris))
    #define __unix 1
#endif


//----------------------------------------------------------------------------
// Unified processor naming
//----------------------------------------------------------------------------

#if defined(DOXYGEN)

    //!
    //! Defined when compiled for a little-endian or LSB-first target platform.
    //!
    #define __little_endian
    //!
    //! Defined when compiled for a big-endian or MSB-first target platform.
    //!
    #define __big_endian
    //!
    //! Defined when the target processor architecture is Intel IA-32, also known as x86.
    //!
    #define __i386
    //!
    //! Defined when the target processor architecture is the 64-bit extension of the
    //! IA-32 architecture, also known as AMD-64 or Intel x86-64.
    //!
    #define __x86_64
    //!
    //! Defined when the target processor architecture is ARM.
    //!
    #define __arm
    //!
    //! Defined when the target processor architecture is STxP70.
    //!
    #define __stxp70
    //!
    //! Defined when the target processor architecture is Intel IA-64 architecture, also known as Itanium.
    //!
    #define __ia64
    //!
    //! Defined when the target processor architecture is 32-bit Power PC.
    //!
    #define __powerpc
    //!
    //! Defined when the target processor architecture is 64-bit Power PC.
    //!
    #define __powerpc64
    //!
    //! Defined when the target processor architecture is Digital Alpha architecture.
    //!
    #define __alpha
    //!
    //! Defined when the target processor architecture is Sun SPARC architecture.
    //!
    #define __sparc

#elif defined(__i386__) || defined(__i386) || defined(_M_IX86)
    #if !defined(__i386)
        #define __i386 1
    #endif
#elif defined(__amd64) || defined(__amd64__) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64)
    #if !defined(__x86_64)
        #define __x86_64 1
    #endif
#elif defined(__ia64__) || defined(_M_IA64)
    #if !defined(__ia64)
        #define __ia64 1
    #endif
#elif defined(__arm__)
    #if !defined(__arm)
        #define __arm 1
    #endif
#elif defined(__stxp70__) || defined(__STxP70__)
    #if !defined(__stxp70)
        #define __stxp70 1
    #endif
#elif defined(__alpha__)
    #if !defined(__alpha)
        #define __alpha 1
    #endif
#elif defined(__sparc__)
    #if !defined(__sparc)
        #define __sparc 1
    #endif
#elif defined(__powerpc__)
    #if !defined(__powerpc)
        #define __powerpc 1
    #endif
#elif defined(__powerpc64__)
    #if !defined(__powerpc64)
        #define __powerpc64 1
    #endif
#else
    #error "New unknown processor, please update tsPlatform.h"
#endif

// Untested platform warnings.
// Should really use "#warning" instead of "#error" but #warning is a
// GCC extension while #error is standard.

#if !defined(__linux) && !defined(__windows) && !defined(__mac)
    #error "TSDuck has been tested on Linux, MacOS and Windows only, review this code"
#endif

#if !defined(__gcc) && !defined(__msc)
    #error "TSDuck has been tested with GCC and MSVC compilers only, review this code"
#endif

// Byte order

#if (defined(__i386) || defined(__x86_64) || defined(__ia64) || defined(__alpha)) && !defined(__little_endian)
    #define __little_endian 1
#elif (defined(__sparc) || defined(__powerpc) || defined(__powerpc64)) && !defined(__big_endian)
    #define __big_endian 1
#endif

#if defined (__arm)
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define __little_endian 1
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define __big_endian 1
    #else
        #error "ARM endianness not defined"
    #endif
#endif

#if !defined(__little_endian) && !defined(__big_endian)
    #error "unknow endian, please update this header file"
#endif


//----------------------------------------------------------------------------
// System-specific settings
//----------------------------------------------------------------------------

// Windows specific settings

#if defined(__windows) && !defined(DOXYGEN)
    #if !defined(WINVER)
        #define WINVER 0x0501            // Allow use of features specific to Windows XP or later.
    #endif
    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0501      // Allow use of features specific to Windows XP or later.
    #endif
    #if defined(UNICODE)
        #undef UNICODE                   // No unicode in TSDuck, use single byte char
    #endif
    #define _CRT_SECURE_NO_DEPRECATE 1
    #define _CRT_NONSTDC_NO_DEPRECATE 1
    #define WIN32_LEAN_AND_MEAN 1        // Exclude rarely-used stuff from Windows headers
#endif

// Large file system (LFS) support on Linux.

#if defined(__linux) && !defined(DOXYGEN)
    #define _LARGEFILE_SOURCE    1
    #define _LARGEFILE64_SOURCE  1
    #define _FILE_OFFSET_BITS   64
#endif

// Enforce assertions, even in optimized mode.

#ifdef NDEBUG
    #undef NDEBUG
#endif

// Disable some Visual C++ warnings.

#if defined(__msc)

// Methods may have unused formal parameters.
// warning C4100 : 'xxx' : unreferenced formal parameter
#pragma warning (disable:4100)

// When a user class is exported in a user DLL and this class has STL templates instantiations
// warning C4251: 'classname' : class 'std::vector<_Ty>' needs to have dll-interface to be used by clients of class 'classname'
#pragma warning (disable:4251)

// When a user class is exported in a user DLL and this class has an STL template instantiation as base class
// warning C4275: non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class 'std::_Container_base_aux_alloc_real<_Alloc>'
#pragma warning (disable:4275)

// VC++ does not implement exception specification
// warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#pragma warning (disable:4290)

// VC++ does not like usage of this in member initializer list although there are some legal usage for that
// (be careful however...)
// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable:4355)

// Depending on VC++ versions, defining _CRT_SECURE_NO_DEPRECATE is not sufficient.
// warning C4995: 'sprintf': name was marked as #pragma deprecated
#pragma warning (disable:4995)

#endif

// System headers

#if defined(__windows)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
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

#if defined(__linux)
#include <sys/mman.h>
#include <byteswap.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <PCSC/reader.h>
#endif

#if defined(__mac)
#include <sys/mman.h>
#include <libproc.h>
#endif

#include <string>
#include <vector>
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
#include <iostream>
#include <exception>

#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstddef>     // size_t
#include <fcntl.h>
#include <winscard.h>  // PC/SC


// Required link libraries under Windows.

#if defined(__windows) && defined(__msc)
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


// Some standard Windows headers have the very-very bad idea
// to define common words as macros

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
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

// Other Microsoft VC oddities....

#if defined(__windows) && !defined(DOXYGEN)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
    #define strcasecmp  _stricmp
    #define strncasecmp _strnicmp
#endif

// For platforms not supporting large files:

#if !defined(__windows) && !defined(O_LARGEFILE) && !defined(DOXYGEN)
    #define O_LARGEFILE 0
#endif

// Identify Linux DVB API version in one value

#if defined(__linux) || defined(DOXYGEN)
    //!
    //! @hideinitializer
    //! On Linux systems, identify the Linux DVB API version in one value.
    //! Example: TS_DVB_API_VERSION is 503 for DVB API version 5.3.
    //!
    #define TS_DVB_API_VERSION ((DVB_API_VERSION * 100) + DVB_API_VERSION_MINOR)
#endif

// Definition of S2API on Linux

#if defined(DOXYGEN)
    //!
    //! Defined when the Linux DVB S2 API is available.
    //!
    #define __s2api
#elif defined(__linux) && defined(DVB_API_VERSION)
    #if DVB_API_VERSION >= 5
        #define __s2api 1
    #endif
#endif

// MacOS has a POSIX-compliant version of strerror_r, returning an int.
// But fails to report this by defining HAVE_INT_STRERROR_R.

#if defined(__mac) && !defined(HAVE_INT_STRERROR_R)
#define HAVE_INT_STRERROR_R 1
#endif

// On MacOS, sigaction(2) uses the flag named SA_RESETHAND instead of SA_ONESHOT.

#if defined(__mac) && !defined(SA_ONESHOT)
#define SA_ONESHOT SA_RESETHAND
#endif


//----------------------------------------------------------------------------
// Basic preprocessing features
//----------------------------------------------------------------------------

// Macro parameter to string transformation

#if !defined(DOXYGEN)
    #define TS_STRINGIFY1(x) #x
#endif
//!
//! @hideinitializer
//! This macro transforms the @e value of a macro parameter into the equivalent string.
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
//! #define P1(v) printf("#parameter:     %s = %d\n", #v, v)
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
//! #parameter:     X = 1
//! TS_STRINGIFY: 1 = 1
//! @endcode
//!
#define TS_STRINGIFY(x) TS_STRINGIFY1(x)

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
#elif defined(__gcc)
    #define TS_UNUSED __attribute__ ((unused))
#elif defined(__msc)
    // With MS compiler, there is no such attribute. It is not possible to disable the
    // "unused" warning for a specific variable. The unused warnings must be disabled.
    // warning C4189: 'xxx' : local variable is initialized but not referenced
    #pragma warning(disable:4189)
    #define TS_UNUSED
#else
    #error "New unknown compiler, please update TS_UNUSED in tsPlatform.h"
#endif

//!
//! Definition of a NULL pointer for C++.
//!
//! C++ normally does not use a @c NULL constant. Null pointers are simply @c 0.
//! However, in untyped context (typically, a variable list of arguments),
//! @c 0 is interpreted as an zero of type @c int and not as a null pointer.
//! On platforms such as x86_64 where @c int and pointers do not have the same size,
//! using @c 0 in a variable list of arguments may produce incorrect results.
//!
//! This header file ensure that @c NULL is defined for both C and C++ and has always
//! the semantic of a null pointer, regardless of the context.
//!
#define TS_NULL (static_cast<void*>(0))

//!
//! Definition of a NULL pointer for C++ with an explicit char pointer.
//! @see TS_NULL
//!
#define TS_NULL_CHAR_PTR (static_cast<char*>(0))

//!
//! Definition of the name of the current function.
//! This is typically __func__ but recent compilers have "pretty" names for C++.
//!
#if defined(DOXYGEN)
    #define TS_FUNCTION
#elif defined(__gcc) && __GNUC__ >= 3
    #define TS_FUNCTION __PRETTY_FUNCTION__
#elif defined(__msc)
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
#if defined(__windows)
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
#if defined(__windows) && defined(_TSDUCKDLL_IMPL)
    #define TSDUCKDLL __declspec(dllexport)
#elif defined(__windows) && defined(_TSDUCKDLL_USE)
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
#if defined(__gcc) || defined(DOXYGEN)
    #define TS_NEED_STATIC_CONST_DEFINITIONS 1
#endif


//----------------------------------------------------------------------------
//  Definition of common integer literals.
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

#elif defined(__msc)
    #define TS_CONST64(n)  n##i64
    #define TS_UCONST64(n) n##ui64
#else
    #define TS_CONST64(n)  (int64_t(n##LL))
    #define TS_UCONST64(n) (uint64_t(n##ULL))
#endif


//----------------------------------------------------------------------------
// Serialization of integer data.
// Suffix BE means serialized data in Big-Endian representation.
// Suffix LE means serialized data in Little-Endian representation.
// No suffix assumes Big-Endian representation.
//----------------------------------------------------------------------------

//!
//! TSDuck namespace, containing all TSDuck classes and functions.
//!
namespace ts {
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
    #if defined(__linux)
        return bswap_16(x);
    #elif defined(__msc)
        return _byteswap_ushort(x);
    #else
        return (x << 8) | (x >> 8);
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
    #if defined(__linux)
        return bswap_32(x);
    #elif defined(__msc)
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
    #if defined(__linux)
        return bswap_64(x);
    #elif defined(__msc)
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
    #if defined(__little_endian)
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
    #if defined(__little_endian)
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
    #if defined(__little_endian)
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
    #if defined(__little_endian)
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
    TSDUCKDLL inline uint16_t CondByteSwap16LE (uint16_t x)
    {
    #if defined(__little_endian)
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
    #if defined(__little_endian)
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
    #if defined(__little_endian)
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
    #if defined(__little_endian)
        return x;
    #else
        return ByteSwap64(x);
    #endif
    }

    //!
    //! Perform a sign extension on 24 bit integers.
    //!
    //! @param [in] x A 32-bit integer containing a signed 24-bit value to extend.
    //! @return A 32-bit signed integer containing the signed 24-bit value with
    //! proper sign extension on 32-bits.
    //!
    TSDUCKDLL inline int32_t SignExtend24(int32_t x)
    {
        return (x & 0x00800000) == 0 ? (x & 0x00FFFFFF) : (x | 0xFF000000);
    }

    //!
    //! Inlined function getting an 8-bit unsigned integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit unsigned integer.
    //! @return The 8-bit unsigned integer at @a p.
    //!
    TSDUCKDLL inline uint8_t GetUInt8(const void* p) {return *(static_cast<const uint8_t*>(p));}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16 (const void* p) {return CondByteSwap16BE(*(static_cast<const uint16_t*>(p)));}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32 (const void* p) {return CondByteSwap32BE(*(static_cast<const uint32_t*>(p)));}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt24 (const void* p) {return GetUInt32(p) >> 8;}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64 (const void* p) {return CondByteSwap64BE(*(static_cast<const uint64_t*>(p)));}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16BE (const void* p) {return CondByteSwap16BE(*(static_cast<const uint16_t*>(p)));}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32BE (const void* p) {return CondByteSwap32BE(*(static_cast<const uint32_t*>(p)));}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt24BE (const void* p) {return GetUInt32BE(p) >> 8;}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64BE (const void* p) {return CondByteSwap64BE(*(static_cast<const uint64_t*>(p)));}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in little endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16LE (const void* p) {return CondByteSwap16LE(*(static_cast<const uint16_t*>(p)));}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in little endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32LE (const void* p) {return CondByteSwap32LE(*(static_cast<const uint32_t*>(p)));}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in little endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt24LE (const void* p) {return GetUInt32LE(static_cast<const uint8_t*>(p) - 1) >> 8;}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in little endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64LE (const void* p) {return CondByteSwap64LE(*(static_cast<const uint64_t*>(p)));}

    //!
    //! Inlined function getting an 8-bit signed integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit signed integer.
    //! @return The 8-bit signed integer at @a p.
    //!
    TSDUCKDLL inline int8_t GetInt8(const void* p) {return *(static_cast<const int8_t*>(p));}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16 (const void* p) {return static_cast<int16_t>(GetUInt16(p));}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @return The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24 (const void* p) {return SignExtend24(static_cast<int32_t>(GetUInt24(p)));}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32 (const void* p) {return static_cast<int32_t>(GetUInt32(p));}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64 (const void* p) {return static_cast<int64_t>(GetUInt64(p));}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16BE (const void* p) {return static_cast<int16_t>(GetUInt16BE(p));}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @return The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24BE (const void* p) {return SignExtend24(static_cast<int32_t>(GetUInt24BE(p)));}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32BE (const void* p) {return static_cast<int32_t>(GetUInt32BE(p));}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64BE (const void* p) {return static_cast<int64_t>(GetUInt64BE(p));}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in little endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16LE (const void* p) {return static_cast<int16_t>(GetUInt16LE(p));}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in little endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24LE (const void* p) {return SignExtend24(static_cast<int32_t>(GetUInt24LE(p)));}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in little endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32LE (const void* p) {return static_cast<int32_t>(GetUInt32LE(p));}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in little endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64LE (const void* p) {return static_cast<int64_t>(GetUInt64LE(p));}

    //!
    //! Inlined function getting an 8-bit unsigned integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit unsigned integer.
    //! @param [out] i The 8-bit unsigned integer at @a p.
    //!
    TSDUCKDLL inline void GetUInt8 (const void* p, uint8_t&  i) {i = GetUInt8(p);}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16 (const void* p, uint16_t& i) {i = GetUInt16(p);}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24 (const void* p, uint32_t& i) {i = GetUInt24(p);}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32 (const void* p, uint32_t& i) {i = GetUInt32(p);}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64 (const void* p, uint64_t& i) {i = GetUInt64(p);}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16BE (const void* p, uint16_t& i) {i = GetUInt16BE(p);}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24BE (const void* p, uint32_t& i) {i = GetUInt24BE(p);}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32BE (const void* p, uint32_t& i) {i = GetUInt32BE(p);}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64BE (const void* p, uint64_t& i) {i = GetUInt64BE(p);}

    //!
    //! Inlined function getting a 16-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in little endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16LE (const void* p, uint16_t& i) {i = GetUInt16LE(p);}

    //!
    //! Inlined function getting a 24-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in little endian representation.
    //! @param [out] i The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24LE (const void* p, uint32_t& i) {i = GetUInt24LE(p);}

    //!
    //! Inlined function getting a 32-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in little endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32LE (const void* p, uint32_t& i) {i = GetUInt32LE(p);}

    //!
    //! Inlined function getting a 64-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in little endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64LE (const void* p, uint64_t& i) {i = GetUInt64LE(p);}

    //!
    //! Inlined function getting an 8-bit signed integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit signed integer.
    //! @param [out] i The 8-bit signed integer at @a p.
    //!
    TSDUCKDLL inline void GetInt8 (const void* p, int8_t&  i) {i = GetInt8(p);}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16 (const void* p, int16_t& i) {i = GetInt16(p);}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @param [out] i The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24 (const void* p, int32_t& i) {i = GetInt24(p);}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32 (const void* p, int32_t& i) {i = GetInt32(p);}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64 (const void* p, int64_t& i) {i = GetInt64(p);}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16BE (const void* p, int16_t& i) {i = GetInt16BE(p);}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @param [out] i The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24BE (const void* p, int32_t& i) {i = GetInt24BE(p);}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32BE (const void* p, int32_t& i) {i = GetInt32BE(p);}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64BE (const void* p, int64_t& i) {i = GetInt64BE(p);}

    //!
    //! Inlined function getting a 16-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in little endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16LE (const void* p, int16_t& i) {i = GetInt16LE(p);}

    //!
    //! Inlined function getting a 24-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in little endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24LE (const void* p, int32_t& i) {i = GetInt24LE(p);}

    //!
    //! Inlined function getting a 32-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in little endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32LE (const void* p, int32_t& i) {i = GetInt32LE(p);}

    //!
    //! Inlined function getting a 64-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in little endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64LE (const void* p, int64_t& i) {i = GetInt64LE(p);}

    //!
    //! Inlined function serializing an 8-bit unsigned integer data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [out] p An address where to serialize the 8-bit unsigned integer.
    //! @param [in]  i The 8-bit unsigned integer to serialize.
    //!
    TSDUCKDLL inline void PutUInt8 (void* p, uint8_t  i) {*(static_cast<uint8_t*>(p)) = i;}

    //!
    //! Inlined function serializing a 16-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt16 (void* p, uint16_t i) {*(static_cast<uint16_t*>(p)) = CondByteSwap16BE(i);}

    //!
    //! Inlined function serializing a 32-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt32 (void* p, uint32_t i) {*(static_cast<uint32_t*>(p)) = CondByteSwap32BE(i);}

    //!
    //! Inlined function serializing a 64-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt64 (void* p, uint64_t i) {*(static_cast<uint64_t*>(p)) = CondByteSwap64BE(i);}

    //!
    //! Inlined function serializing a 16-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt16BE (void* p, uint16_t i) {*(static_cast<uint16_t*>(p)) = CondByteSwap16BE(i);}

    //!
    //! Inlined function serializing a 32-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt32BE (void* p, uint32_t i) {*(static_cast<uint32_t*>(p)) = CondByteSwap32BE(i);}

    //!
    //! Inlined function serializing a 64-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt64BE (void* p, uint64_t i) {*(static_cast<uint64_t*>(p)) = CondByteSwap64BE(i);}

    //!
    //! Inlined function serializing a 16-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt16LE (void* p, uint16_t i) {*(static_cast<uint16_t*>(p)) = CondByteSwap16LE(i);}

    //!
    //! Inlined function serializing a 32-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt32LE (void* p, uint32_t i) {*(static_cast<uint32_t*>(p)) = CondByteSwap32LE(i);}

    //!
    //! Inlined function serializing a 64-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt64LE (void* p, uint64_t i) {*(static_cast<uint64_t*>(p)) = CondByteSwap64LE(i);}

    //!
    //! Inlined function serializing a 24-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 24-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt24BE(void* p, uint32_t i)
    {
        *(static_cast<uint16_t*>(p)) = CondByteSwap16BE(static_cast<uint16_t>(i >> 8));
        *(static_cast<uint8_t*>(p) + 2) = static_cast<uint8_t>(i);
    }

    //!
    //! Inlined function serializing a 24-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 24-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt24(void* p, uint32_t i)
    {
        PutUInt24BE(p, i);
    }

    //!
    //! Inlined function serializing a 24-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt24LE(void* p, uint32_t i)
    {
        *(static_cast<uint16_t*>(p)) = CondByteSwap16LE(static_cast<uint16_t>(i));
        *(static_cast<uint8_t*>(p) + 2) = static_cast<uint8_t>(i >> 16);
    }

    //!
    //! Inlined function serializing an 8-bit signed integer data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [out] p An address where to serialize the 8-bit signed integer.
    //! @param [in]  i The 8-bit signed integer to serialize.
    //!
    TSDUCKDLL inline void PutInt8 (void* p, int8_t  i) {*(static_cast<int8_t*>(p)) = i;}

    //!
    //! Inlined function serializing a 16-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt16 (void* p, int16_t i) {PutUInt16(p, static_cast<uint16_t>(i));}

    //!
    //! Inlined function serializing a 24-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt24 (void* p, int32_t i) {PutUInt24(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 32-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt32 (void* p, int32_t i) {PutUInt32(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 64-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt64 (void* p, int64_t i) {PutUInt64(p, static_cast<uint64_t>(i));}

    //!
    //! Inlined function serializing a 16-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt16BE (void* p, int16_t i) {PutUInt16BE(p, static_cast<uint16_t>(i));}

    //!
    //! Inlined function serializing a 24-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt24BE (void* p, int32_t i) {PutUInt24BE(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 32-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt32BE (void* p, int32_t i) {PutUInt32BE(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 64-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt64BE (void* p, int64_t i) {PutUInt64BE(p, static_cast<uint64_t>(i));}

    //!
    //! Inlined function serializing a 16-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt16LE (void* p, int16_t i) {PutUInt16LE(p, static_cast<uint16_t>(i));}

    //!
    //! Inlined function serializing a 24-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt24LE (void* p, int32_t i) {PutUInt24LE(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 32-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt32LE (void* p, int32_t i) {PutUInt32LE(p, static_cast<uint32_t>(i));}

    //!
    //! Inlined function serializing a 64-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt64LE (void* p, int64_t i) {PutUInt64LE(p, static_cast<uint64_t>(i));}

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT>
    TSDUCKDLL inline INT CondByteSwapBE(INT x)
    {
#if defined(__big_endian)
        return x;
#else
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: assert (false); return 0;
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
    template <typename INT>
    TSDUCKDLL inline INT CondByteSwapLE(INT x)
    {
#if defined(__big_endian)
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: assert (false); return 0;
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
    template <typename INT>
    TSDUCKDLL inline INT CondByteSwap (INT x)
    {
        return CondByteSwapBE<INT>(x);
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline INT GetInt (const void* p)
    {
        return CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline INT GetIntBE (const void* p)
    {
        return CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline INT GetIntLE (const void* p)
    {
        return CondByteSwapLE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline void GetInt (const void* p, INT& i)
    {
        i = CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline void GetIntBE (const void* p, INT& i)
    {
        i = CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT>
    TSDUCKDLL inline void GetIntLE (const void* p, INT& i)
    {
        i = CondByteSwapLE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function serializing an integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT>
    TSDUCKDLL inline void PutInt (void* p, INT i)
    {
        *(static_cast<INT*>(p)) = CondByteSwapBE<INT>(i);
    }

    //!
    //! Template function serializing an integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT>
    TSDUCKDLL inline void PutIntBE (void* p, INT i)
    {
        *(static_cast<INT*>(p)) = CondByteSwapBE<INT>(i);
    }

    //!
    //! Template function serializing an integer data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in little endian representation.
    //!
    template <typename INT>
    TSDUCKDLL inline void PutIntLE (void* p, INT i)
    {
        *(static_cast<INT*>(p)) = CondByteSwapLE<INT>(i);
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
    TSDUCKDLL inline uint32_t ROL (uint32_t word, int i) {return XX;}

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
    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i) {return XX;}

    //!
    //! Inlined function performing 32-bit right-rotate.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROR (uint32_t word, int i) {return XX;}

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
    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i) {return XX;}

#elif defined(__msc)
#pragma intrinsic(_lrotr,_lrotl)

    TSDUCKDLL inline uint32_t ROL (uint32_t word, int i) {return _lrotl (word, i);}
    TSDUCKDLL inline uint32_t ROR (uint32_t word, int i) {return _lrotr (word, i);}

    TSDUCKDLL inline uint32_t ROLc (uint32_t word, const int i) {return _lrotl (word, i);}
    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i) {return _lrotr (word, i);}

#elif defined(__gcc) && (defined(__i386) || defined(__x86_64))

    TSDUCKDLL inline uint32_t ROL (uint32_t word, int i)
    {
        asm ("roll %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR (uint32_t word, int i)
    {
        asm ("rorl %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc (uint32_t word, const int i)
    {
#if defined(DEBUG) || defined(__llvm)
        return ROL (word, i);
#else
        asm ("roll %2,%0"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i)
    {
#if defined(DEBUG) || defined(__llvm)
        return ROR (word, i);
#else
        asm ("rorl %2,%0"
             :"=r" (word)
             :"0" (word),"I" (i));
        return word;
#endif
    }

#elif defined(__powerpc)

    TSDUCKDLL inline uint32_t ROL (uint32_t word, int i)
    {
        asm ("rotlw %0,%0,%2"
             :"=r" (word)
             :"0" (word),"r" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR (uint32_t word, int i)
    {
        asm ("rotlw %0,%0,%2"
             :"=r" (word)
             :"0" (word),"r" (32-i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc (uint32_t word, const int i)
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

    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i)
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

    TSDUCKDLL inline uint32_t ROL (uint32_t word, int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >>(32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROR (uint32_t word, int i)
    {
        return (((word&0xFFFFFFFFUL) >>(i&31)) | (word << (32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROLc (uint32_t word, const int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >>(32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t RORc (uint32_t word, const int i)
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
    TSDUCKDLL inline uint64_t ROL64 (uint64_t word, int i) {return XX;}

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
    TSDUCKDLL inline uint64_t ROL64c (uint64_t word, const int i) {return XX;}

    //!
    //! Inlined function performing 64-bit right-rotate.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROR64 (uint64_t word, int i) {return XX;}

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
    TSDUCKDLL inline uint64_t ROR64c (uint64_t word, const int i) {return XX;}

#elif defined(__gcc) && defined(__x86_64)

    TSDUCKDLL inline uint64_t ROL64 (uint64_t word, int i)
    {
        asm ("rolq %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROR64 (uint64_t word, int i)
    {
        asm ("rorq %%cl,%0"
             :"=r" (word)
             :"0" (word),"c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROL64c (uint64_t word, const int i)
    {
#if defined(DEBUG) || defined(__llvm)
        return ROL64 (word, i);
#else
        asm ("rolq %2,%0"
             :"=r" (word)
             :"0" (word),"J" (i));
        return word;
#endif
    }

    TSDUCKDLL inline uint64_t ROR64c (uint64_t word, const int i)
    {
#if defined(DEBUG) || defined(__llvm)
        return ROR64 (word, i);
#else
        asm ("rorq %2,%0"
             :"=r" (word)
             :"0" (word),"J" (i));
        return word;
#endif
    }

#else

    TSDUCKDLL inline uint64_t ROL64 (uint64_t word, int i)
    {
        return (word << (i&63)) | ((word & TS_UCONST64 (0xFFFFFFFFFFFFFFFF)) >>(64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROR64 (uint64_t word, int i)
    {
        return ((word & TS_UCONST64 (0xFFFFFFFFFFFFFFFF)) >>(i&63)) | (word << (64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROL64c (uint64_t word, const int i)
    {
        return (word << (i&63)) | ((word & TS_UCONST64 (0xFFFFFFFFFFFFFFFF)) >>(64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROR64c (uint64_t word, const int i)
    {
        return ((word & TS_UCONST64 (0xFFFFFFFFFFFFFFFF)) >>(i&63)) | (word << (64-(i&63)));
    }

#endif
}


//----------------------------------------------------------------------------
// Cross-plaftorms portable definitions for memory barrier.
//----------------------------------------------------------------------------

#if defined(__msc)
    #pragma intrinsic(_ReadWriteBarrier)
#endif

#if defined (DOXYGEN) /* documentation only */
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

#elif defined(__gcc) && (defined(__i386) || defined(__x86_64))

        // "mfence" is SSE2, not supported on all x86 cpus but supported on all x86_64 cpus.
        __asm__ __volatile__ ("mfence" : : : "memory");

#elif defined(__gcc) && defined(__arm)

        // For later reference, not sure this is valid.
        unsigned dest = 0;
        __asm__ __volatile__ ("@MemoryBarrier\n mcr p15,0,%0,c7,c10,5\n" : "=&r"(dest) : :  "memory");

#elif defined(__msc)

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
// Printf-like formatting
//----------------------------------------------------------------------------

//!
//! Syntax-checking attribute for functions with a printf-like format.
//!
//! Functions like @c printf use a variable-length argument list.
//! The number of arguments and their required types depend on the content
//! of the @a format string. For each @a format content, there is a specific
//! number of expected arguments and the type of each argument is perfectly
//! defined. But most compilers cannot check the validity of the number of
//! arguments and their types. If the number of arguments or any of their
//! types is wrong, the results are unpredictable. This is very well-known
//! pitfall for C programers.
//!
//! However, some compilers such as GCC are able to analyze printf-like format
//! strings and validate the variable list of arguments at compile time.
//! This is performed using a specific non-portable function attribute.
//!
//! This macro defines a portable way of specifying this attribute. With
//! compilers such as GCC which are able to perform this check, the specific
//! attribute is specified and the compile-time checks are performed. With
//! all other compilers, the macro expands to nothing and no check is performed.
//!
//! @param formatIndex Specifies the index of the @a format argument in the function.
//! @param argIndex Specifies the index of the first argument for the @a format string.
//!
//! Parameter indexes start at 1 for functions and at 2 for C++ member
//! functions (the implicit parameter at index 1 is \c this in the case of
//! C++ member functions).
//!
//! Example:
//! @code
//! void MyPrint(const char *format, ...) TS_PRINTF_FORMAT(1, 2);
//! @endcode
//!
#if defined(DOXYGEN)
    #define TS_PRINTF_FORMAT(formatIndex,argIndex) platform_specific
#elif defined(__gcc) && __GNUC__ >= 4
    #define TS_PRINTF_FORMAT(f,p) __attribute__((format(printf, f, p)))
#else
    #define TS_PRINTF_FORMAT(f,p)
#endif

#if defined(DOXYGEN)
    //!
    //! A string value to specify a @c size_t argument in @c printf @a format strings.
    //!
    //! The size of the predefined type @c size_t depends on the platform.
    //! It is typically 32 or 64 bits. When formatting a @c size_t value
    //! in printf-like functions, the required specifier is consequently
    //! unknown. It can be "%d" or "%ld" or instance. It is difficult to
    //! write portable code which formats a @c size_t value.
    //! Specifically, GCC/glibc and Microsoft C++ use different specifiers.
    //!
    //! This macro translates to a string constant which contains the
    //! appropriate format specifier for @c size_t on the target platform.
    //!
    //! Example:
    //! @code
    //! size_t size = ...;
    //! printf("size_t value is %" FMT_SIZE_T "d", size);
    //! @endcode
    //!
    #define FMT_SIZE_T "platform-specific"
    //!
    //! A string value to specify a 64-bit integer argument in @c printf @a format strings.
    //!
    //! The C/C++ standards do not specify a format for integer types which are
    //! <i>longer than @c long</i>, i.e. 64-bit integers on most platforms.
    //! When formatting a 64-bit integer value in printf-like functions,
    //! the required specifier is consequently unknown.
    //! Specifically, GCC/glibc and Microsoft C++ use different specifiers.
    //!
    //! This macro translates to a string constant which contains the
    //! appropriate format specifier for a 64-bit integer on the target platform.
    //!
    //! Example:
    //! @code
    //! uint64_t ui64 = ...;
    //! printf("64-bit value is %" FMT_INT64 "d", ui64);
    //! @endcode
    //!
    #define FMT_INT64 "platform-specific"

#elif defined(__msc)
    #define FMT_SIZE_T "I"
    #define FMT_INT64  "I64"
#elif defined(__mac)
    #define FMT_SIZE_T "z"
    #define FMT_INT64  "ll"
#elif defined(__gcc) && defined(__x86_64)
    #define FMT_SIZE_T "z"
    #define FMT_INT64  "j"
#elif defined(__gcc)
    #define FMT_SIZE_T "z"
    #define FMT_INT64  "ll"
#else
    #error "check printf/scanf format for size_t and 64-bit integers on this platform"
#endif

//!
//! @hideinitializer
//! This macro is used in functions with a variable list of
//! arguments to format a @c std::string from a printf-like format
//! and its variable-length list of arguments.
//!
//! This operation cannot be done using a function in a portable way.
//! Consequently, a macro is required.
//!
//! Example:
//! @code
//! void f(const char *format, ...)
//! {
//!     std::string result;
//!     TS_FORMAT_STRING(result, format);
//! @endcode
//!
//! @param [in] string_var The @c std::string into which the result is stored.
//! @param [in] format_arg The printf-like format argument of the current function.
//! The variable-length list of arguments for the format are assumed
//! to immediately follow the format argument.
//!
#define TS_FORMAT_STRING(string_var,format_arg)                         \
{                                                                         \
    va_list ap__;                                                         \
    va_start(ap__, format_arg);                                           \
    char buf1__[256];                                                     \
    int size1__ = ::vsnprintf(buf1__, sizeof(buf1__), format_arg, ap__);  \
    va_end(ap__);                                                         \
    if (size1__ < int(sizeof(buf1__))) {                                  \
        string_var = std::string(buf1__, size1__);                        \
    }                                                                     \
    else {                                                                \
        char *buf2__ = new char[size1__ + 1];                             \
        va_start(ap__, format_arg);                                       \
        int size2__ = ::vsnprintf(buf1__, size1__ + 1, format_arg, ap__); \
        va_end(ap__);                                                     \
        assert(size2__ <= size1__);                                       \
        string_var = std::string(buf2__, size2__);                        \
        delete [] buf2__;                                                 \
    }                                                                     \
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
#elif defined(__windows)
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
#elif defined(__windows)
    const ErrorCode SYS_SUCCESS = ERROR_SUCCESS;
#elif defined(__unix)
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
#elif defined(__windows)
    const ErrorCode SYS_DATA_ERROR = ERROR_INVALID_DATA;
#elif defined(__unix)
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
#if defined(__windows)
        return ::GetLastError();
#elif defined(__unix)
        return errno;
#else
        #error "Unsupported operating system"
#endif
    }
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
    //! identical, they should be used explicitely for clarity. Thus,
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
    //! Should be explicitely used for clarity when a variable contains a number of seconds.
    //!
    typedef SubSecond Second;
    //!
    //! This integer type is used to represent a number of milliseconds.
    //! Should be explicitely used for clarity when a variable contains a number of milliseconds.
    //!
    typedef SubSecond MilliSecond;
    //!
    //! This integer type is used to represent a number of microseconds.
    //! Should be explicitely used for clarity when a variable contains a number of microseconds.
    //!
    typedef SubSecond MicroSecond;
    //!
    //! This integer type is used to represent a number of nanoseconds.
    //! Should be explicitely used for clarity when a variable contains a number of nanoseconds.
    //!
    typedef SubSecond NanoSecond;
    //!
    //! This constant shall be used by convention to express an infinite
    //! number of sub-quantities of seconds.
    //!
    const SubSecond Infinite = TS_CONST64 (0x7FFFFFFFFFFFFFFF);
    //!
    //! Number of nanoseconds per second
    //!
    const NanoSecond NanoSecPerSec = 1000000000;
    //!
    //! Number of nanoseconds per millisecond
    //!
    const NanoSecond NanoSecPerMilliSec = 1000000;
    //!
    //! Number of nanoseconds per microsecond
    //!
    const NanoSecond NanoSecPerMicroSec = 1000;
    //!
    //! Number of microseconds per second
    //!
    const MicroSecond MicroSecPerSec = 1000000;
    //!
    //! Number of microseconds per millisecond
    //!
    const MicroSecond MicroSecPerMilliSec = 1000;
    //!
    //! Number of milliseconds per second
    //!
    const MilliSecond MilliSecPerSec = 1000;
    //!
    //! Number of milliseconds per minute
    //!
    const MilliSecond MilliSecPerMin = 1000 * 60;
    //!
    //! Number of milliseconds per hour
    //!
    const MilliSecond MilliSecPerHour = 1000 * 60 * 60;
    //!
    //! Number of milliseconds per day
    //!
    const MilliSecond MilliSecPerDay = 1000 * 60 * 60 * 24;
}


//----------------------------------------------------------------------------
//  General purpose enumeration types
//----------------------------------------------------------------------------

namespace ts {
    //!
    //! Enumeration type used to indicate if the data referenced by a pointer shall be copied or shared.
    enum CopyShare {
        COPY,  //!< Data shall be copied.
        SHARE  //!< Data shall be shared.
    };
}

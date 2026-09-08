//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Safely include the libsrt definitions.
//!
//!  By default, the standard Haivision SRT library is used.
//!
//!  As an alternative, the Robotweax SRT library can be used. First, it must
//!  be built and installed in some directory. Then build TSDuck with:
//!
//!    make ROBOTWEAX_SRT_DIR=/where/robotweax-srt/is/installed
//!
//!  This definition translates to:
//!
//!  - Compiler: -DTS_USE_ROBOTWEAX_SRT -I$ROBOTWEAX_SRT_DIR/include
//!  - Linker: $ROBOTWEAX_SRT_DIR/lib/libsrt.a -lcrypto
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if !defined(TS_NO_SRT)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(documentation)
    TS_LLVM_NOWARNING(old-style-cast)
    TS_LLVM_NOWARNING(undef)
    TS_GCC_NOWARNING(undef)
    TS_GCC_NOWARNING(effc++)
    TS_MSC_NOWARNING(4005)  // 'xxx' : macro redefinition
    TS_MSC_NOWARNING(4668)  // 'xxx' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
    TS_MSC_NOWARNING(4865)  // 'xxx' : the underlying type will change from 'int' to 'unsigned int' when '/Zc:enumTypes' is specified on the command line

    // Bug in GCC: "#if __APPLE__" triggers -Werror=undef despite TS_GCC_NOWARNING(undef)
    // This is a known GCC bug since 2012, never fixed: #if is too early in lex analysis and #pragma are not yet parsed.
    // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
    #if defined(TS_GCC_ONLY) && !defined(__APPLE__)
        #define __APPLE__ 0
        #define ZERO__APPLE__ 1
    #endif

    // On macOS, srt/platform_sys.h unconditionally defines __APPLE_USE_RFC_3542.
    // Since we also define it, avoid "macro redefined" errors.
    #if defined(__APPLE_USE_RFC_3542)
        #undef __APPLE_USE_RFC_3542
    #endif

    #include <srt/version.h>

    // On earlier versions, the header srt.h uses a [[deprecated]] attribute on a typedef, which is incorrect.
    // Disable using [[deprecated]].
    #if SRT_VERSION_VALUE < SRT_MAKE_VERSION_VALUE(1,4,2)
        #define SRT_NO_DEPRECATED 1
    #endif

    // Now include the libsrt headers.
    #include <srt/srt.h>

    // If we want to use Robotweax SRT, make sure we included the right header, not the one from libsrt.
    #if defined(TS_USE_ROBOTWEAX_SRT) && !defined(ROBOTWEAX_SRT_COMPAT_SRT_H) && !defined(ROBOTWEAX_SRT_FLAT_COMPAT_HEADER_H)
        #error "Configured to use Robotweax SRT but seems to use the standard srt/srt.h"
    #endif

    // The header access_control.h was introduced in version 1.4.2.
    // On Windows, access_control.h was missing in the binary installer before 1.5.3.
    #if SRT_VERSION_VALUE < SRT_MAKE_VERSION_VALUE(1,4,2)
        using SRT_RejectReason = SRT_REJECT_REASON;
    #else
        #define HAS_SRT_ACCESS_CONTROL 1
        using SRT_RejectReason = int;
        #if defined(TS_WINDOWS) && SRT_VERSION_VALUE < SRT_MAKE_VERSION_VALUE(1,5,3)
            #define SRT_REJX_OVERLOAD 1402 // manually defined when header is missing.
        #else
            #include <srt/access_control.h>
            // Similar sanity check if we use Robotweax SRT.
            #if defined(TS_USE_ROBOTWEAX_SRT) && !defined(ROBOTWEAX_SRT_COMPAT_ACCESS_CONTROL_H)
                #error "Configured to use Robotweax SRT but seems to use the standard srt/access_control.h"
            #endif
        #endif
    #endif

    #if defined(ZERO__APPLE__)
        #undef __APPLE__
        #undef ZERO__APPLE__
    #endif

    #if defined(TS_MAC) && !defined(__APPLE_USE_RFC_3542)
        #define __APPLE_USE_RFC_3542 1
    #endif

    TS_POP_WARNING()
#endif

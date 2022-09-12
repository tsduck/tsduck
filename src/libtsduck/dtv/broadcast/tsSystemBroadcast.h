//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2022, Thierry Lelegard
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
//!  @ingroup hardware
//!  Include the definitions for broadcast devices in the operating system.
//!
//----------------------------------------------------------------------------

#pragma once

#include "tsBeforeStandardHeaders.h"
#if defined(TS_LINUX)
    #include <linux/dvb/version.h>
    #include <linux/dvb/frontend.h>
    #include <linux/dvb/dmx.h>
    #include <linux/version.h>
#elif defined(TS_WINDOWS)
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
#endif
#include "tsAfterStandardHeaders.h"

// Identify Linux DVB API version in one value
#if defined(TS_LINUX) || defined(DOXYGEN)
    //!
    //! @hideinitializer
    //! On Linux systems, identify the Linux DVB API version in one value.
    //! Example: TS_DVB_API_VERSION is 503 for DVB API version 5.3.
    //!
    #define TS_DVB_API_VERSION ((DVB_API_VERSION * 100) + DVB_API_VERSION_MINOR)
#endif

// Required link libraries under Windows.
#if defined(TS_WINDOWS) && defined(TS_MSC)
    #pragma comment(lib, "quartz.lib")
#endif

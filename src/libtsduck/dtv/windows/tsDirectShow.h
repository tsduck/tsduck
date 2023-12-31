//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup windows
//!  Include the definitions for DirectShow (Windows media framework).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#include "tsBeforeStandardHeaders.h"
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
#include "tsAfterStandardHeaders.h"

// Required link libraries.
#if defined(TS_MSC)
    #pragma comment(lib, "quartz.lib")
#endif

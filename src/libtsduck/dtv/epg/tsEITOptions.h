//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  EIT generation options.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! EIT generation options.
    //! The options can be specified as a byte mask.
    //!
    enum class EITOptions {
        GEN_NONE          = 0x0000,   //!< Generate nothing.
        GEN_ACTUAL        = 0x0001,   //!< Generate EIT actual.
        GEN_OTHER         = 0x0002,   //!< Generate EIT other.
        GEN_PF            = 0x0004,   //!< Generate EIT present/following.
        GEN_SCHED         = 0x0008,   //!< Generate EIT schedule.
        GEN_ALL           = 0x000F,   //!< Generate all EIT's.
        LOAD_INPUT        = 0x0010,   //!< Use input EIT's as EPG data.
        PACKET_STUFFING   = 0x0020,   //!< Insert stuffing inside TS packet at end of EIT section. Do not pack EIT sections.
        LAZY_SCHED_UPDATE = 0x0040,   //!< Do not update current EIT schedule section when an event is completed.
        SYNC_VERSIONS     = 0x0080,   //!< Keep version numbers synchronous on all sections of a subtable.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::EITOptions);

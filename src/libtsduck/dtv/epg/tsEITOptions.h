//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        GEN_ACTUAL_PF     = 0x0001,   //!< Generate EIT actual present/following.
        GEN_OTHER_PF      = 0x0002,   //!< Generate EIT other present/following.
        GEN_ACTUAL_SCHED  = 0x0004,   //!< Generate EIT actual schedule.
        GEN_OTHER_SCHED   = 0x0008,   //!< Generate EIT other schedule.
        GEN_PF            = 0x0003,   //!< Generate all EIT present/following.
        GEN_SCHED         = 0x000C,   //!< Generate all EIT schedule.
        GEN_ACTUAL        = 0x0005,   //!< Generate all EIT actual.
        GEN_OTHER         = 0x000A,   //!< Generate all EIT other.
        GEN_ALL           = 0x000F,   //!< Generate all EIT's.
        LOAD_INPUT        = 0x0010,   //!< Use input EIT's as EPG data.
        PACKET_STUFFING   = 0x0020,   //!< Insert stuffing inside TS packet at end of EIT section. Do not pack EIT sections.
        LAZY_SCHED_UPDATE = 0x0040,   //!< Do not update current EIT schedule section when an event is completed.
        SYNC_VERSIONS     = 0x0080,   //!< Keep version numbers synchronous on all sections of a subtable.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::EITOptions);

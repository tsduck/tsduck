//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic ISDB definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIntegerMap.h"
#include "tsTS.h"

namespace ts {

    // Name of section of ISDB-T layer short names.
    // Must be a string object, cannot be a literal, to be used at NTTP (C++ limitation).
    //! @cond nodoxygen
    TSDUCKDLL extern const UString ISDBTLayerCounterNamesSection;
    //! @endcond

    //!
    //! A map of packet counters, indexed by ISDB-T layer.
    //!
    using ISDBTLayerCounter = IntegerMap<uint8_t, PacketCounter, &ISDBTLayerCounterNamesSection>;
}

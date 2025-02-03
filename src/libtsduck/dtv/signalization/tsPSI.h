//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Common definitions for MPEG PSI (Program Specific Information) layer.
//!  Also contains definitions for DVB SI (Service Information) and ATSC.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Maximum size of a descriptor (255 + 2-byte header).
    //!
    constexpr size_t MAX_DESCRIPTOR_SIZE = 257;

    //!
    //! Header size of a short section.
    //!
    constexpr size_t SHORT_SECTION_HEADER_SIZE = 3;

    //!
    //! Header size of a long section.
    //!
    constexpr size_t LONG_SECTION_HEADER_SIZE = 8;

    //!
    //! Size of the CRC32 field in a long section.
    //!
    constexpr size_t SECTION_CRC32_SIZE = 4;

    //!
    //! Maximum size of a PSI section (MPEG-defined).
    //!
    constexpr size_t MAX_PSI_SECTION_SIZE = 1024;

    //!
    //! Maximum size of a private section (including DVB-defined sections).
    //!
    constexpr size_t MAX_PRIVATE_SECTION_SIZE = 4096;

    //!
    //! Minimum size of a short section.
    //!
    constexpr size_t MIN_SHORT_SECTION_SIZE = SHORT_SECTION_HEADER_SIZE;

    //!
    //! Minimum size of a long section.
    //!
    constexpr size_t MIN_LONG_SECTION_SIZE = LONG_SECTION_HEADER_SIZE + SECTION_CRC32_SIZE;

    //!
    //! Maximum size of the payload of a short section.
    //!
    constexpr size_t MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE = MAX_PSI_SECTION_SIZE - SHORT_SECTION_HEADER_SIZE;

    //!
    //! Maximum size of the payload of a PSI long section.
    //!
    constexpr size_t MAX_PSI_LONG_SECTION_PAYLOAD_SIZE = MAX_PSI_SECTION_SIZE - LONG_SECTION_HEADER_SIZE - SECTION_CRC32_SIZE;

    //!
    //! Maximum size of the payload of a private short section.
    //!
    constexpr size_t MAX_PRIVATE_SHORT_SECTION_PAYLOAD_SIZE = MAX_PRIVATE_SECTION_SIZE - SHORT_SECTION_HEADER_SIZE;

    //!
    //! Maximum size of the payload of a private long section.
    //!
    constexpr size_t MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE = MAX_PRIVATE_SECTION_SIZE - LONG_SECTION_HEADER_SIZE - SECTION_CRC32_SIZE;

    //!
    //! Size (in bits) of a section version field.
    //!
    constexpr size_t SVERSION_BITS = 5;

    //!
    //! Mask to wrap a section version value.
    //! Section version values wrap at 32.
    //!
    constexpr uint8_t SVERSION_MASK = 0x1F;

    //!
    //! Maximum value of a section version.
    //!
    constexpr uint8_t SVERSION_MAX = 1 << SVERSION_BITS;

    //!
    //! A placeholder for "invalid transport stream id" value.
    //! In theory, all 16-bit values can be valid TS id. However, this one is "usually" not used.
    //!
    constexpr uint16_t INVALID_TS_ID = 0xFFFF;

    //!
    //! A placeholder for "invalid service id" value.
    //! In theory, all 16-bit values can be valid service id. However, this one is "usually" not used.
    //!
    constexpr uint16_t INVALID_SERVICE_ID = 0xFFFF;
}

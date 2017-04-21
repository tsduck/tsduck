//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Names of various MPEG entities (namespace ts::names)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsCASFamily.h"

namespace ts {
    //!
    //! Namespace for functions returning MPEG/DVB names.
    //!
    namespace names {

        // Table ID. Use CAS family for EMM/ECM table ids.
        TSDUCKDLL std::string TID (uint8_t tid, CASFamily cas = CAS_OTHER);

        // Descriptor ID. Use private data specified (pds) if did >= 0x80.
        TSDUCKDLL std::string DID (uint8_t did, uint32_t pds = 0);

        // Extended descriptor ID. Use private data specified (pds) if edid >= 0x80.
        TSDUCKDLL std::string EDID (uint8_t edid, uint32_t pds = 0);

        // Private Data Specified
        TSDUCKDLL std::string PrivateDataSpecifier (uint32_t);

        // Stream type (in PMT)
        TSDUCKDLL std::string StreamType (uint8_t);

        // Stream ID (in PES header)
        TSDUCKDLL std::string StreamId (uint8_t);

        // PES start code value
        TSDUCKDLL std::string PESStartCode (uint8_t);

        // Aspect ratio values (in MPEG-1/2 video sequence header)
        TSDUCKDLL std::string AspectRatio (uint8_t);

        // Chroma format values (in MPEG-1/2 video sequence header)
        TSDUCKDLL std::string ChromaFormat (uint8_t);

        // AVC (ISO 14496-10, ITU H.264) access unit (aka "NALunit") type
        TSDUCKDLL std::string AVCUnitType (uint8_t);

        // AVC (ISO 14496-10, ITU H.264) profile
        TSDUCKDLL std::string AVCProfile (int);

        // Service type (in Service Descriptor)
        TSDUCKDLL std::string ServiceType (uint8_t);

        // Linkage type (in Linkage Descriptor)
        TSDUCKDLL std::string LinkageType (uint8_t);

        // Subtitling type (in Subtitling Descriptor)
        TSDUCKDLL std::string SubtitlingType (uint8_t);

        // Teletext type (in Teletext Descriptor)
        TSDUCKDLL std::string TeletextType (uint8_t);

        // Conditional Access System Id (in CA Descriptor)
        TSDUCKDLL std::string CASId (uint16_t);

        // Running Status (in SDT)
        TSDUCKDLL std::string RunningStatus (uint8_t);

        // Audio type (in ISO639 Language Descriptor)
        TSDUCKDLL std::string AudioType (uint8_t);

        // Component Type (in Component Descriptor)
        // Combination of stream_content (4 bits) and component_type (8 bits)
        TSDUCKDLL std::string ComponentType (uint16_t);

        // AC-3 Component Type
        TSDUCKDLL std::string AC3ComponentType (uint8_t);

        // DTS Audio Sample Rate code
        TSDUCKDLL std::string DTSSampleRateCode (uint8_t);

        // DTS Audio Bit Rate Code
        TSDUCKDLL std::string DTSBitRateCode (uint8_t);

        // DTS Audio Surround Mode
        TSDUCKDLL std::string DTSSurroundMode (uint8_t);

        // DTS Audio Extended Surround Mode
        TSDUCKDLL std::string DTSExtendedSurroundMode (uint8_t);

        // Content name (in Content Descriptor)
        TSDUCKDLL std::string Content (uint8_t);

        // Scrambling control value in TS header
        TSDUCKDLL std::string ScramblingControl (uint8_t);

        // Bouquet Id
        TSDUCKDLL std::string BouquetId (uint16_t);

        // Data broadcast id (in Data Broadcast Id Descriptor)
        TSDUCKDLL std::string DataBroadcastId (uint16_t);

        // Binary data type (in Binary Data Table)
        TSDUCKDLL std::string BinaryDataType (uint8_t);

        // Binary data format (in Binary Data Table)
        TSDUCKDLL std::string BinaryDataFormat (uint8_t);

        // OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits
        TSDUCKDLL std::string OUI (uint32_t);
    }
}

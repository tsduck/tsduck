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
        //!
        //! Name of Table ID.
        //! @param [in] tid Table id.
        //! @param [in] cas CAS family for EMM/ECM table ids.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string TID(uint8_t tid, CASFamily cas = CAS_OTHER);

        //!
        //! Name of Descriptor ID.
        //! @param [in] did Descriptor ID.
        //! @param [in] pds Private data specified if @a did >= 0x80.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DID(uint8_t did, uint32_t pds = 0);

        //!
        //! Name of Extended descriptor ID.
        //! @param [in] edid Extended descriptor ID.
        //! @param [in] pds Private data specified if @a edid >= 0x80.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string EDID(uint8_t edid, uint32_t pds = 0);

        //!
        //! Name of Private Data Specifier.
        //! @param [in] pds Private Data Specifier.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string PrivateDataSpecifier(uint32_t pds);

        //!
        //! Name of Stream type (in PMT).
        //! @param [in] st Stream type (in PMT).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string StreamType(uint8_t st);

        //!
        //! Name of Stream ID (in PES header).
        //! @param [in] sid Stream ID (in PES header).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string StreamId(uint8_t sid);

        //!
        //! Name of PES start code value.
        //! @param [in] code PES start code value.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string PESStartCode(uint8_t code);

        //!
        //! Name of aspect ratio values (in MPEG-1/2 video sequence header).
        //! @param [in] a Aspect ratio value (in MPEG-1/2 video sequence header).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string AspectRatio(uint8_t a);

        //!
        //! Name of Chroma format values (in MPEG-1/2 video sequence header).
        //! @param [in] c Chroma format value (in MPEG-1/2 video sequence header).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string ChromaFormat(uint8_t c);

        //!
        //! Name of AVC (ISO 14496-10, ITU H.264) access unit (aka "NALunit") type.
        //! @param [in] ut AVC access unit type.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string AVCUnitType(uint8_t ut);

        //!
        //! Name of AVC (ISO 14496-10, ITU H.264) profile.
        //! @param [in] p AVC profile.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string AVCProfile(int p);

        //!
        //! Name of service type (in Service Descriptor).
        //! @param [in] st Service type (in Service Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string ServiceType(uint8_t st);

        //!
        //! Name of linkage type (in Linkage Descriptor).
        //! @param [in] lt Linkage type (in Linkage Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string LinkageType(uint8_t lt);

        //!
        //! Name of subtitling type (in Subtitling Descriptor).
        //! @param [in] st Subtitling type (in Subtitling Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string SubtitlingType(uint8_t st);

        //!
        //! Name of Teletext type (in Teletext Descriptor).
        //! @param [in] tt Teletext type (in Teletext Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string TeletextType(uint8_t tt);

        //!
        //! Name of Conditional Access System Id (in CA Descriptor).
        //! @param [in] casid Conditional Access System Id (in CA Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string CASId(uint16_t casid);

        //!
        //! Name of Running Status (in SDT).
        //! @param [in] rs Running Status (in SDT).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string RunningStatus(uint8_t rs);

        //!
        //! Name of audio type (in ISO639 Language Descriptor).
        //! @param [in] at Audio type (in ISO639 Language Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string AudioType(uint8_t at);

        //!
        //! Name of Component Type (in Component Descriptor).
        //! @param [in] ct Component Type (in Component Descriptor).
        //! Combination of stream_content (4 bits) and component_type (8 bits).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string ComponentType(uint16_t ct);

        //!
        //! Name of AC-3 Component Type.
        //! @param [in] t AC-3 Component Type.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string AC3ComponentType(uint8_t t);

        //!
        //! Name of DTS Audio Sample Rate code.
        //! @param [in] c DTS Audio Sample Rate code.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DTSSampleRateCode(uint8_t c);

        //!
        //! Name of DTS Audio Bit Rate Code.
        //! @param [in] c DTS Audio Bit Rate Code.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DTSBitRateCode(uint8_t c);

        //!
        //! Name of DTS Audio Surround Mode.
        //! @param [in] mode DTS Audio Surround Mode.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DTSSurroundMode(uint8_t mode);

        //!
        //! Name of DTS Audio Extended Surround Mode.
        //! @param [in] mode DTS Audio Extended Surround Mode.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DTSExtendedSurroundMode(uint8_t mode);

        //!
        //! Name of content name (in Content Descriptor).
        //! @param [in] c Content name.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string Content(uint8_t c);

        //!
        //! Name of scrambling control value in TS header
        //! @param [in] sc Scrambling control value in TS header
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string ScramblingControl(uint8_t sc);

        //!
        //! Name of Bouquet Id.
        //! @param [in] id Bouquet Id.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string BouquetId(uint16_t id);

        //!
        //! Name of Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] id Data broadcast id (in Data Broadcast Id Descriptor).
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string DataBroadcastId(uint16_t id);

        //!
        //! Name of OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits.
        //! @param [in] oui Organizationally Unique Identifier), 24 bits.
        //! @return The corresponding name.
        //!
        TSDUCKDLL std::string OUI(uint32_t oui);
    }
}

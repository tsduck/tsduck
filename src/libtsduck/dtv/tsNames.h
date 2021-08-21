//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  @ingroup mpeg
//!  Names of various MPEG entities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNamesFile.h"
#include "tsPSI.h"
#include "tsEnumUtils.h"
#include "tsCASFamily.h"
#include "tsCodecType.h"

namespace ts {

    class DuckContext;

    //!
    //! Namespace for functions returning MPEG/DVB names.
    //!
    namespace names {
        //!
        //! Get the NamesFile instance for all MPEG/DVB names.
        //! @return A pointer to the NamesFile instance for all MPEG/DVB names.
        //!
        inline const NamesFile* File()
        {
            return NamesFile::Instance(NamesFile::Predefined::DTV);
        }

        //!
        //! Name of Table ID.
        //! @param [in] duck TSDuck execution context (used to select from conflicting standards).
        //! @param [in] tid Table id.
        //! @param [in] cas CAS id for EMM/ECM table ids.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString TID(const DuckContext& duck, uint8_t tid, uint16_t cas = CASID_NULL, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Descriptor ID.
        //! @param [in] did Descriptor id.
        //! @param [in] pds Private data specified if @a did >= 0x80.
        //! @param [in] tid Optional id of the enclosing table.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DID(uint8_t did, uint32_t pds = 0, uint8_t tid = 0xFF, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Check if a descriptor id has a specific name for a given table.
        //! @param [in] did Descriptor id.
        //! @param [in] tid Table id of the enclosing table.
        //! @return True if descriptor @a did has a specific name for table @a tid.
        //!
        TSDUCKDLL bool HasTableSpecificName(uint8_t did, uint8_t tid);

        //!
        //! Name of Extended descriptor ID.
        //! @param [in] edid Extended descriptor ID.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString EDID(uint8_t edid, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Private Data Specifier.
        //! @param [in] pds Private Data Specifier.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PrivateDataSpecifier(uint32_t pds, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Stream type (in PMT).
        //! @param [in] st Stream type (in PMT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString StreamType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Stream ID (in PES header).
        //! @param [in] sid Stream ID (in PES header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString StreamId(uint8_t sid, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of PES start code value.
        //! @param [in] code PES start code value.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PESStartCode(uint8_t code, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of aspect ratio values (in MPEG-1/2 video sequence header).
        //! @param [in] a Aspect ratio value (in MPEG-1/2 video sequence header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AspectRatio(uint8_t a, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Chroma format values (in MPEG-1/2 video sequence header).
        //! @param [in] c Chroma format value (in MPEG-1/2 video sequence header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ChromaFormat(uint8_t c, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of AVC/HEVC/VVC access unit (aka "NALunit") type.
        //! @param [in] codec One of AVC, HEVC, VVC.
        //! @param [in] ut Access unit type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AccessUnitType(CodecType codec, uint8_t ut, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of AVC (ISO 14496-10, ITU H.264) profile.
        //! @param [in] p AVC profile.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AVCProfile(int p, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of service type (in Service Descriptor).
        //! @param [in] st Service type (in Service Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ServiceType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of linkage type (in Linkage Descriptor).
        //! @param [in] lt Linkage type (in Linkage Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString LinkageType(uint8_t lt, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of subtitling type (in Subtitling Descriptor).
        //! @param [in] st Subtitling type (in Subtitling Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString SubtitlingType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Teletext type (in Teletext Descriptor).
        //! @param [in] tt Teletext type (in Teletext Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString TeletextType(uint8_t tt, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Conditional Access System Id (in CA Descriptor).
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] casid Conditional Access System Id (in CA Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString CASId(const DuckContext& duck, uint16_t casid, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Conditional Access Families.
        //! @param [in] cas CAS family
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString CASFamily(ts::CASFamily cas);

        //!
        //! Name of Running Status (in SDT).
        //! @param [in] rs Running Status (in SDT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString RunningStatus(uint8_t rs, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of audio type (in ISO639 Language Descriptor).
        //! @param [in] at Audio type (in ISO639 Language Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AudioType(uint8_t at, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Component Type (in Component Descriptor).
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] ct Component Type (in Component Descriptor).
        //! Combination of stream_content_ext (4 bits), stream_content (4 bits) and component_type (8 bits).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ComponentType(const DuckContext& duck, uint16_t ct, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of AC-3 Component Type.
        //! @param [in] t AC-3 Component Type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AC3ComponentType(uint8_t t, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of DTS Audio Sample Rate code.
        //! @param [in] c DTS Audio Sample Rate code.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSSampleRateCode(uint8_t c, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of DTS Audio Bit Rate Code.
        //! @param [in] c DTS Audio Bit Rate Code.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSBitRateCode(uint8_t c, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of DTS Audio Surround Mode.
        //! @param [in] mode DTS Audio Surround Mode.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSSurroundMode(uint8_t mode, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of DTS Audio Extended Surround Mode.
        //! @param [in] mode DTS Audio Extended Surround Mode.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSExtendedSurroundMode(uint8_t mode, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of content name (in Content Descriptor).
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] c Content name.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString Content(const DuckContext& duck, uint8_t c, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of scrambling control value in TS header
        //! @param [in] sc Scrambling control value in TS header
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ScramblingControl(uint8_t sc, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Bouquet Id.
        //! @param [in] id Bouquet Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString BouquetId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Original Network Id.
        //! @param [in] id Original Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString OriginalNetworkId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Network Id.
        //! @param [in] id Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString NetworkId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Platform Id.
        //! @param [in] id Platform Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PlatformId(uint32_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] id Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DataBroadcastId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits.
        //! @param [in] oui Organizationally Unique Identifier), 24 bits.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString OUI(uint32_t oui, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of T2-MI packet type.
        //! @param [in] type T2-MI packet type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString T2MIPacketType(uint8_t type, NamesFlags flags = NamesFlags::NAME);
    }

    //!
    //! Get a name from a specified section in the DVB names file.
    //! @tparam INT An integer name.
    //! @param [in] sectionName Name of section to search. Not case-sensitive.
    //! @param [in] value Value to get the name for.
    //! @param [in] flags Presentation flags.
    //! @param [in] bits Nominal size in bits of the data, optional.
    //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    UString NameFromSection(const UString& sectionName, INT value, NamesFlags flags = NamesFlags::NAME, size_t bits = 0, INT alternateValue = 0)
    {
        return names::File()->nameFromSection(sectionName, NamesFile::Value(value), flags, bits, NamesFile::Value(alternateValue));
    }
}

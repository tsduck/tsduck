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

#include "tsNames.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Tables ids: specific standards processing
//----------------------------------------------------------------------------

ts::UString ts::names::TID(const DuckContext& duck, uint8_t tid, uint16_t cas, NamesFlags flags)
{
    // Where to search table ids.
    const NamesFile* const repo = File();
    const UString section(u"TableId");

    // Check without standard, then with all known standards in TSDuck context.
    // In all cases, use version with CAS first, then without CAS.
    // Return the first name which is found.
    // If no name is found in the list of supported standards but some with
    // other standards, use the first one that was found.

    const NamesFile::Value casMask = NamesFile::Value(CASFamilyOf(cas)) << 8;
    NamesFile::Value finalValue = NamesFile::Value(tid);

    if (repo->nameExists(section, finalValue | casMask)) {
        // Found without standard, with CAS.
        finalValue |= casMask;
    }
    else if (repo->nameExists(section, finalValue)) {
        // Found without standard, without CAS. Nothing to do. Keep this value.
    }
    else {
        // Loop on all possible standards.
        bool foundOnce = false;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            // TID value with mask for this standard:
            const NamesFile::Value value = NamesFile::Value(tid) | (NamesFile::Value(mask) << 16);
            // Check if this standard is currently in TSDuck context.
            const bool supportedStandard = bool(duck.standards() & mask);
            // Lookup name only if supported standard or no previous standard was found.
            if (!foundOnce || supportedStandard) {
                bool foundHere = repo->nameExists(section, value | casMask);
                if (foundHere) {
                    // Found with that standard, with CAS.
                    finalValue = value | casMask;
                    foundOnce = true;
                }
                else if (repo->nameExists(section, value)) {
                    // Found with that standard, without CAS.
                    finalValue = value;
                    foundHere = foundOnce = true;
                }
                if (foundHere && supportedStandard) {
                    break;
                }
            }
        }
    }

    // Return the name for best matched value.
    return repo->nameFromSection(section, finalValue, flags, 8);
}


//----------------------------------------------------------------------------
// Descriptor ids: specific processing for table-specific descriptors.
//----------------------------------------------------------------------------

bool ts::names::HasTableSpecificName(uint8_t did, uint8_t tid)
{
    return tid != TID_NULL &&
        did < 0x80 &&
        File()->nameExists(u"DescriptorId", (NamesFile::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | NamesFile::Value(did));
}

ts::UString ts::names::DID(uint8_t did, uint32_t pds, uint8_t tid, NamesFlags flags)
{
    if (did >= 0x80 && pds != 0 && pds != PDS_NULL) {
        // If this is a private descriptor, only consider the private value.
        // Do not fallback because the same value with PDS == 0 can be different.
        return File()->nameFromSection(u"DescriptorId", (NamesFile::Value(pds) << 8) | NamesFile::Value(did), flags, 8);
    }
    else if (tid != 0xFF) {
        // Could be a table-specific descriptor.
        const NamesFile::Value fullValue = (NamesFile::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | NamesFile::Value(did);
        return File()->nameFromSectionWithFallback(u"DescriptorId", fullValue, NamesFile::Value(did), flags, 8);
    }
    else {
        return File()->nameFromSection(u"DescriptorId", NamesFile::Value(did), flags, 8);
    }
}

//----------------------------------------------------------------------------
// Public functions returning names.
//----------------------------------------------------------------------------

ts::UString ts::names::EDID(uint8_t edid, NamesFlags flags)
{
    return File()->nameFromSection(u"DVBExtendedDescriptorId", NamesFile::Value(edid), flags, 8);
}

ts::UString ts::names::StreamType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"StreamType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::PrivateDataSpecifier(uint32_t pds, NamesFlags flags)
{
    return File()->nameFromSection(u"PrivateDataSpecifier", NamesFile::Value(pds), flags, 32);
}

ts::UString ts::names::CASFamily(ts::CASFamily cas)
{
    return File()->nameFromSection(u"CASFamily", NamesFile::Value(cas), NamesFlags::NAME | NamesFlags::DECIMAL);
}

ts::UString ts::names::CASId(const DuckContext& duck, uint16_t id, NamesFlags flags)
{
    const UChar* section = bool(duck.standards() & Standards::ISDB) ? u"ARIBCASystemId" : u"CASystemId";
    return File()->nameFromSection(section, NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::BouquetId(uint16_t id, NamesFlags flags)
{
    return File()->nameFromSection(u"BouquetId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::OriginalNetworkId(uint16_t id, NamesFlags flags)
{
    return File()->nameFromSection(u"OriginalNetworkId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::NetworkId(uint16_t id, NamesFlags flags)
{
    return File()->nameFromSection(u"NetworkId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::PlatformId(uint32_t id, NamesFlags flags)
{
    return File()->nameFromSection(u"PlatformId", NamesFile::Value(id), flags, 24);
}

ts::UString ts::names::DataBroadcastId(uint16_t id, NamesFlags flags)
{
    return File()->nameFromSection(u"DataBroadcastId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::OUI(uint32_t oui, NamesFlags flags)
{
    return NamesFile::Instance(NamesFile::Predefined::OUI)->nameFromSection(u"OUI", NamesFile::Value(oui), flags, 24);
}

ts::UString ts::names::StreamId(uint8_t sid, NamesFlags flags)
{
    return File()->nameFromSection(u"StreamId", NamesFile::Value(sid), flags, 8);
}

ts::UString ts::names::PESStartCode(uint8_t code, NamesFlags flags)
{
    return File()->nameFromSection(u"PESStartCode", NamesFile::Value(code), flags, 8);
}

ts::UString ts::names::AspectRatio(uint8_t ar, NamesFlags flags)
{
    return File()->nameFromSection(u"AspectRatio", NamesFile::Value(ar), flags, 8);
}

ts::UString ts::names::ChromaFormat(uint8_t cf, NamesFlags flags)
{
    return File()->nameFromSection(u"ChromaFormat", NamesFile::Value(cf), flags, 8);
}

ts::UString ts::names::AccessUnitType(CodecType codec, uint8_t type, NamesFlags flags)
{
    const UChar* table = nullptr;
    if (codec == CodecType::AVC) {
        table = u"AVCUnitType";
    }
    else if (codec == CodecType::HEVC) {
        table = u"HEVCUnitType";
    }
    else if (codec == CodecType::VVC) {
        table = u"VVCUnitType";
    }
    if (table != nullptr) {
        return File()->nameFromSection(table, NamesFile::Value(type), flags, 8);
    }
    else {
        return NamesFile::Formatted(NamesFile::Value(type), u"unknown", flags, 8);
    }
}

ts::UString ts::names::AVCProfile(int profile, NamesFlags flags)
{
    return File()->nameFromSection(u"AVCProfile", NamesFile::Value(profile), flags, 8);
}

ts::UString ts::names::ServiceType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"ServiceType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::LinkageType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"LinkageType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::TeletextType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"TeletextType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::RunningStatus(uint8_t status, NamesFlags flags)
{
    return File()->nameFromSection(u"RunningStatus", NamesFile::Value(status), flags, 8);
}

ts::UString ts::names::AudioType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"AudioType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::SubtitlingType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"SubtitlingType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::DTSSampleRateCode(uint8_t x, NamesFlags flags)
{
    return File()->nameFromSection(u"DTSSampleRate", NamesFile::Value(x), flags, 8);
}

ts::UString ts::names::DTSBitRateCode(uint8_t x, NamesFlags flags)
{
    return File()->nameFromSection(u"DTSBitRate", NamesFile::Value(x), flags, 8);
}

ts::UString ts::names::DTSSurroundMode(uint8_t x, NamesFlags flags)
{
    return File()->nameFromSection(u"DTSSurroundMode", NamesFile::Value(x), flags, 8);
}

ts::UString ts::names::DTSExtendedSurroundMode(uint8_t x, NamesFlags flags)
{
    return File()->nameFromSection(u"DTSExtendedSurroundMode", NamesFile::Value(x), flags, 8);
}

ts::UString ts::names::ScramblingControl(uint8_t scv, NamesFlags flags)
{
    return File()->nameFromSection(u"ScramblingControl", NamesFile::Value(scv), flags, 8);
}

ts::UString ts::names::T2MIPacketType(uint8_t type, NamesFlags flags)
{
    return File()->nameFromSection(u"T2MIPacketType", NamesFile::Value(type), flags, 8);
}


//----------------------------------------------------------------------------
// Component Type (in Component Descriptor)
//----------------------------------------------------------------------------

ts::UString ts::names::ComponentType(const DuckContext& duck, uint16_t type, NamesFlags flags)
{
    // There is a special case here. The binary layout of the 16 bits are:
    //   stream_content_ext (4 bits)
    //   stream_content (4 bits)
    //   component_type (8 bits).
    //
    // In the beginning, stream_content_ext did not exist and, as a reserved
    // field, was 0xF. Starting with stream_content > 8, stream_content_ext
    // appeared and may have different values. Logically, stream_content_ext
    // is a subsection of stream_content. So, the bit order for values in the
    // name file is stream_content || stream_content_ext || component_type.
    //
    // We apply the following transformations:
    // - The lookup the name, we use stream_content || stream_content_ext || component_type.
    // - To display the value, we use the real binary value where stream_content_ext
    //   is forced to zero when stream_content is in the range 1 to 8.

    // Stream content:
    const uint16_t sc = (type & 0x0F00) >> 8;

    // Value to use for name lookup:
    const uint16_t nType = (sc >= 1 && sc <= 8 ? 0x0F00 : ((type & 0xF000) >> 4)) | uint16_t((type & 0x0F00) << 4) | (type & 0x00FF);

    // Value to display:
    const uint16_t dType = sc >= 1 && sc <= 8 ? (type & 0x0FFF) : type;

    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return File()->nameFromSection(u"ComponentTypeJapan", NamesFile::Value(nType), flags | NamesFlags::ALTERNATE, 16, dType);
    }
    else if ((nType & 0xFF00) == 0x3F00) {
        return SubtitlingType(nType & 0x00FF, flags);
    }
    else if ((nType & 0xFF00) == 0x4F00) {
        return AC3ComponentType(nType & 0x00FF, flags);
    }
    else {
        return File()->nameFromSection(u"ComponentType", NamesFile::Value(nType), flags | NamesFlags::ALTERNATE, 16, dType);
    }
}


//----------------------------------------------------------------------------
// Content ids with variants.
//----------------------------------------------------------------------------

ts::UString ts::names::Content(const DuckContext& duck, uint8_t x, NamesFlags flags)
{
    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return File()->nameFromSection(u"ContentIdJapan", NamesFile::Value(x), flags, 8);
    }
    else if (bool(duck.standards() & Standards::ABNT)) {
        // ABNT (Brazil) / ISDB uses a completely different mapping.
        return File()->nameFromSection(u"ContentIdABNT", NamesFile::Value(x), flags, 8);
    }
    else {
        // Standard DVB mapping.
        return File()->nameFromSection(u"ContentId", NamesFile::Value(x), flags, 8);
    }
}


//----------------------------------------------------------------------------
// AC-3 Component Type, field-based, no buildin list of values
//----------------------------------------------------------------------------

ts::UString ts::names::AC3ComponentType(uint8_t type, NamesFlags flags)
{
    ts::UString s((type & 0x80) != 0 ? u"Enhanced AC-3" : u"AC-3");

    s += (type & 0x40) != 0 ? u", full" : u", combined";

    switch (type & 0x38) {
        case 0x00: s += u", complete main"; break;
        case 0x08: s += u", music and effects"; break;
        case 0x10: s += u", visually impaired"; break;
        case 0x18: s += u", hearing impaired"; break;
        case 0x20: s += u", dialogue"; break;
        case 0x28: s += u", commentary"; break;
        case 0x30: s += u", emergency"; break;
        case 0x38: s += (type & 0x40) ? u", karaoke" : u", voiceover"; break;
        default: assert(false); // unreachable
    }

    switch (type & 0x07) {
        case 0: s += u", mono"; break;
        case 1: s += u", 1+1 channel"; break;
        case 2: s += u", 2 channels"; break;
        case 3: s += u", 2 channels dolby surround"; break;
        case 4: s += u", multichannel > 2"; break;
        case 5: s += u", multichannel > 5.1"; break;
        case 6: s += u", multiple substreams"; break;
        case 7: s += u", reserved"; break;
        default: assert(false); // unreachable
    }

    return NamesFile::Formatted(type, s, flags, 8);
}

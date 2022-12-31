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

#include "tsNames.h"
#include "tsCASFamily.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Tables ids: specific standards processing
//----------------------------------------------------------------------------

ts::UString ts::names::TID(const DuckContext& duck, uint8_t tid, uint16_t cas, NamesFlags flags)
{
    // Where to search table ids.
    const NamesFile* const repo = NamesFile::Instance(NamesFile::Predefined::DTV);
    const UString section(u"TableId");
    const NamesFile::Value casValue = NamesFile::Value(CASFamilyOf(cas)) << 8;
    const NamesFile::Value tidValue = NamesFile::Value(tid);

    if (repo->nameExists(section, tidValue | casValue)) {
        // Found without standard, with CAS.
        return repo->nameFromSection(section, tidValue | casValue, flags, 8);
    }
    else if (repo->nameExists(section, tidValue)) {
        // Found without standard, without CAS. Keep this value.
        return repo->nameFromSection(section, tidValue, flags, 8);
    }
    else {
        // Loop on all possible standards. Build a list of possible names.
        UStringList allNames;
        bool foundWithSupportedStandard = false;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            // Check if this standard is currently in TSDuck context.
            const bool supportedStandard = bool(duck.standards() & mask);
            const NamesFile::Value stdValue = NamesFile::Value(mask) << 16;
            // Lookup name only if supported standard or no previous standard was found.
            if (!foundWithSupportedStandard || supportedStandard) {
                UString name;
                if (repo->nameExists(section, tidValue | stdValue | casValue)) {
                    // Found with that standard and CAS.
                    name = repo->nameFromSection(section, tidValue | stdValue | casValue, flags, 8);
                }
                else if (repo->nameExists(section, tidValue | stdValue)) {
                    // Found with that standard, without CAS.
                    name = repo->nameFromSection(section, tidValue | stdValue, flags, 8);
                }
                if (!name.empty()) {
                    // A name was found.
                    if (!foundWithSupportedStandard && supportedStandard) {
                        // At least one supported standard is found.
                        // Clear previous results without supported standard.
                        // Will no longer try without supported standard.
                        foundWithSupportedStandard = true;
                        allNames.clear();
                    }
                    allNames.push_back(name);
                }
            }
        }
        if (allNames.empty()) {
            // No name found, use default formatting with the value only.
            return repo->nameFromSection(section, tidValue, flags, 8);
        }
        else {
            // One or more possibility. Return them all since we cannot choose.
            return UString::Join(allNames, u" or ");
        }
    }
}


//----------------------------------------------------------------------------
// Descriptor ids: specific processing for table-specific descriptors.
//----------------------------------------------------------------------------

bool ts::names::HasTableSpecificName(uint8_t did, uint8_t tid)
{
    const NamesFile* const repo = NamesFile::Instance(NamesFile::Predefined::DTV);
    return tid != TID_NULL &&
        did < 0x80 &&
        repo->nameExists(u"DescriptorId", (NamesFile::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | NamesFile::Value(did));
}

ts::UString ts::names::DID(uint8_t did, uint32_t pds, uint8_t tid, NamesFlags flags)
{
    if (did >= 0x80 && pds != 0 && pds != PDS_NULL) {
        // If this is a private descriptor, only consider the private value.
        // Do not fallback because the same value with PDS == 0 can be different.
        return NameFromDTV(u"DescriptorId", (NamesFile::Value(pds) << 8) | NamesFile::Value(did), flags, 8);
    }
    else if (tid != 0xFF) {
        // Could be a table-specific descriptor.
        const NamesFile::Value fullValue = (NamesFile::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | NamesFile::Value(did);
        return NameFromDTVWithFallback(u"DescriptorId", fullValue, NamesFile::Value(did), flags, 8);
    }
    else {
        return NameFromDTV(u"DescriptorId", NamesFile::Value(did), flags, 8);
    }
}

//----------------------------------------------------------------------------
// Public functions returning names.
//----------------------------------------------------------------------------

ts::UString ts::names::EDID(uint8_t edid, NamesFlags flags)
{
    return NameFromDTV(u"DVBExtendedDescriptorId", NamesFile::Value(edid), flags, 8);
}

ts::UString ts::names::StreamType(uint8_t type, NamesFlags flags)
{
    return NameFromDTV(u"StreamType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::PrivateDataSpecifier(uint32_t pds, NamesFlags flags)
{
    return NameFromDTV(u"PrivateDataSpecifier", NamesFile::Value(pds), flags, 32);
}

ts::UString ts::names::CASId(const DuckContext& duck, uint16_t id, NamesFlags flags)
{
    const UChar* section = bool(duck.standards() & Standards::ISDB) ? u"ARIBCASystemId" : u"CASystemId";
    return NameFromDTV(section, NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::BouquetId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"BouquetId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::OriginalNetworkId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"OriginalNetworkId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::NetworkId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"NetworkId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::DataBroadcastId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"DataBroadcastId", NamesFile::Value(id), flags, 16);
}

ts::UString ts::names::ServiceType(uint8_t type, NamesFlags flags)
{
    return NameFromDTV(u"ServiceType", NamesFile::Value(type), flags, 8);
}

ts::UString ts::names::RunningStatus(uint8_t status, NamesFlags flags)
{
    return NameFromDTV(u"RunningStatus", NamesFile::Value(status), flags, 8);
}


//----------------------------------------------------------------------------
// Content ids with variants.
//----------------------------------------------------------------------------

ts::UString ts::names::Content(const DuckContext& duck, uint8_t x, NamesFlags flags)
{
    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return NameFromDTV(u"ContentIdJapan", NamesFile::Value(x), flags, 8);
    }
    else if (bool(duck.standards() & Standards::ABNT)) {
        // ABNT (Brazil) / ISDB uses a completely different mapping.
        return NameFromDTV(u"ContentIdABNT", NamesFile::Value(x), flags, 8);
    }
    else {
        // Standard DVB mapping.
        return NameFromDTV(u"ContentId", NamesFile::Value(x), flags, 8);
    }
}

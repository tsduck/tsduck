 //----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDID.h"
#include "tsEDID.h"


//----------------------------------------------------------------------------
// Name of a Descriptor ID.
//----------------------------------------------------------------------------

ts::UString ts::DIDName(DID did, NamesFlags flags, const DescriptorContext& context)
{
    // Get all possible EDID values for that descriptor.
    const ts::NamesFile::NamesFilePtr repo = ts::NamesFile::Instance(ts::NamesFile::Predefined::DTV);
    std::set<NamesFile::Value> values;
    repo->valuesFromSection(values, u"DescriptorId", did);

    // Select the most appropriate EDID.
    NamesFile::Value final_edid = did;
    bool found = values.empty();

    const Standards standards = context.getStandards();
    const TID tid = context.getTableId();

    // Table-specific descriptors have the highest priority.
    if (!found && tid != TID_NULL) {
        for (auto v : values) {
            if (EDID(v).tableId() == tid) {
                final_edid = v;
                found = true;
                break;
            }
        }
    }

    // Then, look for private descriptor.
    if (!found) {
        REGID regid = REGID_NULL;
        PDS pds = PDS_NULL;
        REGID* pregid = nullptr;
        PDS* ppds = nullptr;
        // It is probably costless to walk though the small set of possible value than searching private
        // descriptors in the environment. So, check first if there is a private descriptor.
        for (auto v : values) {
            if (EDID(v).isPrivateMPEG()) {
                pregid = &regid;
            }
            if (bool(standards & Standards::DVB) && EDID(v).isPrivateDVB()) {
                ppds = &pds;
            }
        }
        if (pregid != nullptr || ppds != nullptr) {
            // This DID is possibly the one of a private descriptor, search the private identifiers in the context.
            bool regid_first = context.getPrivateIds(pregid, ppds);
            if (regid != REGID_NULL && (regid_first || pds == PDS_NULL)) {
                for (auto v : values) {
                    if (EDID(v).regid() == regid) {
                        final_edid = v;
                        found = true;
                        break;
                    }
                }
            }
            if (!found && pds != PDS_NULL) {
                for (auto v : values) {
                    if (EDID(v).pds() == pds) {
                        final_edid = v;
                        found = true;
                        break;
                    }
                }
            }
            if (!found && regid != REGID_NULL && !regid_first && pds != PDS_NULL) {
                for (auto v : values) {
                    if (EDID(v).regid() == regid) {
                        final_edid = v;
                        found = true;
                        break;
                    }
                }
            }
        }
    }

    // Then, look for a regular descriptor.
    if (!found) {
        NamesFile::Value match = 0;
        size_t match_count = 0;
        for (auto v : values) {
            const EDID edid(v);
            if (edid.isRegular()) {
                const Standards edid_std = edid.standards();
                if ((edid_std | standards) == Standards::NONE) {
                    // No standard in the context, no standard in the EDID, any DID is a potential match.
                    match = v;
                    match_count++;
                }
                else if (bool(edid_std & standards)) {
                    // There is an exact standard match.
                    final_edid = v;
                    found = true;
                    break;
                }
            }
        }
        if (!found && match_count == 1) {
            // No exact match but exactly one possible match, keep it.
            final_edid = match;
        }
    }

    return repo->nameFromSection(u"DescriptorId", final_edid, flags);
}


//----------------------------------------------------------------------------
// Check if a descriptor id has a specific name for a given table.
//----------------------------------------------------------------------------

bool ts::DIDHasTableSpecificName(DID did, TID tid)
{
    if (tid != TID_NULL) {
        std::set<NamesFile::Value> values;
        NamesFile::Instance(NamesFile::Predefined::DTV)->valuesFromSection(values, u"DescriptorId", did);
        for (auto val : values) {
            if (EDID(val).tableId() == tid) {
                return true;
            }
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Name of an extension descriptor ID.
//----------------------------------------------------------------------------

namespace {
    ts::UString XDIDName(ts::DID xdid, ts::EDID edid, ts::NamesFlags flags)
    {
        const ts::NamesFile::NamesFilePtr repo = ts::NamesFile::Instance(ts::NamesFile::Predefined::DTV);
        if (repo->nameExists(u"DescriptorId", edid.encoded())) {
            return repo->nameFromSection(u"DescriptorId", edid.encoded(), flags | ts::NamesFlags::ALTERNATE, xdid);
        }
        else {
            return ts::NamesFile::Formatted(xdid, ts::UString(), flags, 8);
        }
    }
}

ts::UString ts::XDIDNameMPEG(DID xdid, NamesFlags flags)
{
    return XDIDName(xdid, EDID::ExtensionMPEG(xdid), flags);
}

ts::UString ts::XDIDNameDVB(DID xdid, NamesFlags flags)
{
    return XDIDName(xdid, EDID::ExtensionDVB(xdid), flags);
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEDID.h"


//----------------------------------------------------------------------------
// Build the EDID for an MPEG or DVB extension descriptor.
//----------------------------------------------------------------------------

ts::EDID ts::EDID::Extension(XDID xdid)
{
    return xdid.isExtensionMPEG() ? ExtensionMPEG(xdid.xdid()) : (xdid.isExtensionDVB() ? ExtensionDVB(xdid.xdid()) : Regular(xdid.did(), Standards::NONE));
}


//----------------------------------------------------------------------------
// Build the EDID for a table-specific descriptor.
//----------------------------------------------------------------------------

ts::EDID ts::EDID::TableSpecific(DID did, Standards std, TID tid1, TID tid2, TID tid3, TID tid4)
{
    // Sort the tid values.
    std::array<TID, 4> buf{tid1, tid2, tid3, tid4};
    std::sort(buf.begin(), buf.end());

    return EDID((uint64_t(Type::TABLE_SPEC) << 40) |
                (uint64_t(std) << 48) |
                (uint64_t(buf[3] & 0xFF) << 32) |
                (uint64_t(buf[2] & 0xFF) << 24) |
                (uint64_t(buf[1] & 0xFF) << 16) |
                (uint64_t(buf[0] & 0xFF) << 8) |
                (did & 0xFF));
}


//----------------------------------------------------------------------------
// Check if the descriptor is table-specific and matches a given table id.
//----------------------------------------------------------------------------

bool ts::EDID::matchTableSpecific(TID tid, Standards std) const
{
    // If standards are specified in the context and in the EDID, they must have at least one in common.
    if (tid != TID_NULL && isTableSpecific() && (!std || !standards() || bool(std & standards()))) {
        for (int shift = 8; shift < 40; shift += 8) {
            const TID tidx = TID((_edid >> shift) & 0xFF);
            if (tidx == tid) {
                return true;
            }
            if (tidx == TID_NULL) {
                return false;
            }
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Get the required table-ids for a table-specific descriptor.
//----------------------------------------------------------------------------

std::set<ts::TID> ts::EDID::tableIds() const
{
    std::set<TID> tids;
    if (isTableSpecific()) {
        for (int shift = 8; shift < 40; shift += 8) {
            const TID tid = TID((_edid >> shift) & 0xFF);
            if (tid == TID_NULL) {
                break;
            }
            tids.insert(tid);
        }
    }
    return tids;
}


//----------------------------------------------------------------------------
// Check if the descriptor is a regular one and matches at least one standard.
//----------------------------------------------------------------------------

bool ts::EDID::matchRegularStandards(Standards std) const
{
    // If the regular descriptor has declared no standard, then it matches by default.
    return isRegular() && CompatibleStandards(std | standards());
}


//----------------------------------------------------------------------------
// Build an eXtension Descriptor Id from the EDID.
//----------------------------------------------------------------------------

ts::XDID ts::EDID::xdid() const
{
    return XDID(DID(_edid & 0xFF), type() == Type::EXTENDED ? DID((_edid >> 8) & 0xFF) : DID(XDID_NULL));
}


//----------------------------------------------------------------------------
// Convert to a string object.
//----------------------------------------------------------------------------

ts::UString ts::EDID::toString() const
{
    UString s;
    s.format(u"DID: %X", did());
    switch (type()) {
        case Type::REGULAR: {
            s.append(u", regular");
            break;
        }
        case Type::PRIVATE: {
            s.format(u", private: %X", privateId());
            break;
        }
        case Type::EXTENDED: {
            s.format(u", extension: %X", didExtension());
            break;
        }
        case Type::TABLE_SPEC: {
            s.append(u", table-specific: ");
            const UChar* sep = u"";
            for (int shift = 8; shift < 40; shift += 8) {
                const TID tid = TID((_edid >> shift) & 0xFF);
                if (tid == TID_NULL) {
                    break;
                }
                s.format(u"%s%X", sep, tid);
                sep = u", ";
            }
            break;
        }
        case Type::INVALID:
        default: {
            s.format(u", invalid: ", _edid);
            break;
        }
    }
    s.format(u", std: %s", StandardsNames(standards()));
    return s;
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCIOR.h"

//----------------------------------------------------------------------------
// IOR
//----------------------------------------------------------------------------
namespace {
    // CDR rule (CORBA encoding): variable-length opaque fields are padded to
    // a 4-byte boundary. Both readers and writers must honor it.
    constexpr size_t CDR_ALIGN = 4;

    void writeCDRPad(ts::PSIBuffer& buf, size_t length)
    {
        if (length % CDR_ALIGN != 0) {
            const size_t pad = CDR_ALIGN - (length % CDR_ALIGN);
            for (size_t i = 0; i < pad; ++i) {
                buf.putUInt8(0);
            }
        }
    }

    void skipCDRPad(ts::PSIBuffer& buf, size_t length)
    {
        if (length % CDR_ALIGN != 0) {
            buf.skipBytes(CDR_ALIGN - (length % CDR_ALIGN));
        }
    }
}


void ts::DSMCCIOR::clear()
{
    type_id.clear();
    tagged_profiles.clear();
}

void ts::DSMCCIOR::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(uint32_t(type_id.size()));
    buf.putBytes(type_id);
    writeCDRPad(buf, type_id.size());

    buf.putUInt32(uint32_t(tagged_profiles.size()));

    for (const auto& tagged_profile : tagged_profiles) {
        tagged_profile.serialize(buf);
    }
}

void ts::DSMCCIOR::deserialize(PSIBuffer& buf)
{
    const uint32_t type_id_length = buf.getUInt32();
    buf.getBytes(type_id, type_id_length);
    skipCDRPad(buf, type_id_length);

    const uint32_t tagged_profiles_count = buf.getUInt32();

    for (size_t i = 0; i < tagged_profiles_count; i++) {
        tagged_profiles.emplace_back().deserialize(buf);
    }
}

void ts::DSMCCIOR::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    const uint32_t type_id_length = buf.getUInt32();

    ByteBlock type_id {};
    buf.getBytes(type_id, type_id_length);
    skipCDRPad(buf, type_id_length);

    disp.displayVector(u"Type id: ", type_id, margin);

    const uint32_t tagged_profiles_count = buf.getUInt32();

    for (size_t i = 0; i < tagged_profiles_count; i++) {
        DSMCCTaggedProfile::Display(disp, buf, margin);
    }
}

void ts::DSMCCIOR::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* ior = parent->addElement(u"IOR");
    ior->addHexaTextChild(u"type_id", type_id, true);
    for (const auto& tagged_profile : tagged_profiles) {
        tagged_profile.toXML(duck, ior);
    }
}

bool ts::DSMCCIOR::fromXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getHexaTextChild(type_id, u"type_id");

    for (auto& xprofile : element->children(u"tagged_profile", &ok)) {
        ok = ok && tagged_profiles.emplace_back().fromXML(duck, &xprofile);
    }

    return ok;
}

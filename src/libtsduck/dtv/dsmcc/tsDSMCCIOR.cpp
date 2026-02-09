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

void ts::DSMCCIOR::clear()
{
    type_id.clear();
    tagged_profiles.clear();
}

void ts::DSMCCIOR::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(uint32_t(type_id.size()));
    buf.putBytes(type_id);

    if (type_id.size() % 4 != 0) {
        for (size_t i = 0; i < 4 - (type_id.size() % 4); ++i) {
            buf.putUInt8(0);
        }
    }

    buf.putUInt32(uint32_t(tagged_profiles.size()));

    for (const auto& tagged_profile : tagged_profiles) {
        tagged_profile.serialize(buf);
    }
}

void ts::DSMCCIOR::deserialize(PSIBuffer& buf)
{
    const uint32_t type_id_length = buf.getUInt32();

    for (size_t i = 0; i < type_id_length; i++) {
        type_id.appendUInt8(buf.getUInt8());
    }

    // CDR alignment rule
    if (type_id_length % 4 != 0) {
        buf.skipBytes(4 - (type_id_length % 4));
    }

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

    if (type_id_length % 4 != 0) {
        buf.skipBytes(4 - (type_id_length % 4));
    }

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
        ok = tagged_profiles.emplace_back().fromXML(duck, &xprofile);
    }

    return ok;
}

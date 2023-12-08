//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBTargetRegionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_target_region_descriptor"
#define MY_CLASS ts::ISDBTargetRegionDescriptor
#define MY_DID ts::DID_ISDB_TARGET_REGION
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBTargetRegionDescriptor::ISDBTargetRegionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBTargetRegionDescriptor::ISDBTargetRegionDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBTargetRegionDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBTargetRegionDescriptor::clearContent()
{
    region_spec_type = 0;
    target_region_mask.reset();
}

void ts::ISDBTargetRegionDescriptor::PrefectureMap::clear()
{
    for (auto & p : prefectures) {
        p = false;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBTargetRegionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(region_spec_type);
    if (region_spec_type == 0x01 && target_region_mask.has_value()) {
        target_region_mask.value().serialize(buf);
    }
}

void ts::ISDBTargetRegionDescriptor::PrefectureMap::serialize(PSIBuffer& buf) const
{
    for (auto prefecture : prefectures)
        buf.putBit(prefecture);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBTargetRegionDescriptor::deserializePayload(PSIBuffer& buf)
{
    region_spec_type = buf.getUInt8();
    if (region_spec_type == 0x01) {
        PrefectureMap tmp(buf);
        target_region_mask = tmp;
    }
}

void ts::ISDBTargetRegionDescriptor::PrefectureMap::deserialize(PSIBuffer& buf)
{
    clear();
    for (size_t i = 0; i < MAX_PREFECTURES; i++) {
        prefectures[i] = buf.getBool();
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBTargetRegionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        uint8_t _region_spec_type = buf.getUInt8();
        disp << margin << DataName(MY_XML_NAME, u"region_spec_type", _region_spec_type) << std::endl;
        if (_region_spec_type == 0x01) {
            PrefectureMap map;
            map.display(disp, buf, margin);
        }
    }
}

void ts::ISDBTargetRegionDescriptor::PrefectureMap::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    constexpr auto LINE_WIDTH = 80;
    deserialize(buf);
    disp << margin << "Prefectures: " << toString() << std::endl;
    uint8_t count = 0;
    size_t  output_pos = 0;
    disp << margin;
    for (size_t prefecture = 1; prefecture <= MAX_PREFECTURES; prefecture++) {
        if (prefectures[prefecture - 1]) {
            count++;
            UString print_name = DataName(MY_XML_NAME, u"region", prefecture-1);
            if ((margin.length() + output_pos + print_name.length() + 2) > LINE_WIDTH) {
                disp << std::endl << margin;
                output_pos = 0;
            }
            disp << print_name << u"; ";
            output_pos += print_name.length() + 2;
        }
    }
    if (count == 0) {
        disp << "  -no regions specified-";
    }
    disp << std::endl;
}

ts::UString ts::ISDBTargetRegionDescriptor::PrefectureMap::toString() const
{
    UString ret;
    for (auto prefecture : prefectures) {
        ret += prefecture ? u"1" : u"0";
    }
    return ret;
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBTargetRegionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"region_spec_type", region_spec_type, true);
    if (region_spec_type == 0x01 && target_region_mask.has_value()) {
        target_region_mask.value().toXML(root);
    }
}

void ts::ISDBTargetRegionDescriptor::PrefectureMap::toXML(xml::Element* root) const
{
    root->setAttribute(u"regions_mask", toString());
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBTargetRegionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(region_spec_type, u"region_spec_type", true);
    if (ok && (region_spec_type == 0x01)) {
        PrefectureMap tmp;
        ok = tmp.fromXML(element);
        if (ok) {
            target_region_mask = tmp;
        }
    }
    return ok;
}

bool ts::ISDBTargetRegionDescriptor::PrefectureMap::fromXML(const xml::Element* element)
{
    UString _mask;
    bool ok = element->getAttribute(_mask, u"regions_mask", true, u"", MAX_PREFECTURES, MAX_PREFECTURES);

    if (ok) {
        for (uint8_t prefecture = 0; prefecture < MAX_PREFECTURES; prefecture++) {
            if (_mask.at(prefecture) == u'1') {
                prefectures[prefecture] = true;
            }
        }
    }
    return ok;
}

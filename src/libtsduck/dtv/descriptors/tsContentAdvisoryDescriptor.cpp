//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsContentAdvisoryDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"content_advisory_descriptor"
#define MY_CLASS ts::ContentAdvisoryDescriptor
#define MY_DID ts::DID_ATSC_CONTENT_ADVIS
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentAdvisoryDescriptor::ContentAdvisoryDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ContentAdvisoryDescriptor::clearContent()
{
    entries.clear();
}

ts::ContentAdvisoryDescriptor::ContentAdvisoryDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentAdvisoryDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(entries.size(), 6);
    for (const auto& it1 : entries) {
        buf.putUInt8(it1.rating_region);
        buf.putUInt8(uint8_t(it1.rating_values.size()));
        for (const auto& it2 : it1.rating_values) {
            buf.putUInt8(it2.first);     // rating_dimension_j
            buf.putBits(0xFF, 4);
            buf.putBits(it2.second, 4);  // rating_value
        }
        buf.putMultipleStringWithLength(it1.rating_description);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    const size_t reg_count = buf.getBits<size_t>(6);
    for (size_t i1 = 0; i1 < reg_count && buf.canRead(); ++i1) {
        Entry entry;
        entry.rating_region = buf.getUInt8();
        const size_t dim_count = buf.getUInt8();
        for (size_t i2 = 0; i2 < dim_count && buf.canRead(); ++i2) {
            const uint8_t dim = buf.getUInt8();
            buf.skipBits(4);
            buf.getBits(entry.rating_values[dim], 4);
        }
        buf.getMultipleStringWithLength(entry.rating_description);
        entries.push_back(entry);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(2);
        size_t reg_count = buf.getBits<size_t>(6);
        disp << margin << "Number of regions: " << reg_count << std::endl;
        while (buf.canReadBytes(2) && reg_count > 0) {
            disp << margin << UString::Format(u"- Rating region: 0x%X (%<d)", {buf.getUInt8()});
            size_t dim_count = buf.getUInt8();
            disp << UString::Format(u", number of dimensions: %d", {dim_count}) << std::endl;
            while (buf.canReadBytes(2) && dim_count > 0) {
                disp << margin << UString::Format(u"    Rating dimension j: 0x%X (%<d)", {buf.getUInt8()});
                buf.skipBits(4);
                disp << UString::Format(u", rating value: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
                dim_count--;
            }
            if (buf.canReadBytes(1)) {
                disp.displayATSCMultipleString(buf, 1, margin + u"  ", u"Rating description: ");
            }
            reg_count--;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : entries) {
        xml::Element* e = root->addElement(u"region");
        e->setIntAttribute(u"rating_region", it1.rating_region, true);
        for (auto it2 : it1.rating_values) {
            xml::Element* e2 = e->addElement(u"dimension");
            e2->setIntAttribute(u"rating_dimension_j", it2.first, true);
            e2->setIntAttribute(u"rating_value", it2.second, true);
        }
        it1.rating_description.toXML(duck, e, u"rating_description", true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContentAdvisoryDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"region", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        xml::ElementVector children2;
        ok = children[i]->getIntAttribute(entry.rating_region, u"rating_region", true) &&
             children[i]->getChildren(children2, u"dimension", 0, 255) &&
             entry.rating_description.fromXML(duck, children[i], u"rating_description", false);
        for (size_t i2 = 0; ok && i2 < children2.size(); ++i2) {
            uint8_t dim = 0;
            uint8_t val = 0;
            ok = children2[i2]->getIntAttribute(dim, u"rating_dimension_j", true) &&
                 children2[i2]->getIntAttribute(val, u"rating_value", true, 0, 0, 0x0F);
            entry.rating_values[dim] = val;
        }
        entries.push_back(entry);
    }
    return ok;
}

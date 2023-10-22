//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLocalTimeOffsetDescriptor.h"
#include "tsDescriptor.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"local_time_offset_descriptor"
#define MY_CLASS ts::LocalTimeOffsetDescriptor
#define MY_DID ts::DID_LOCAL_TIME_OFFSET
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor(DuckContext& duck, const Descriptor& desc) :
    LocalTimeOffsetDescriptor()
{
    deserialize(duck, desc);
}

void ts::LocalTimeOffsetDescriptor::clearContent()
{
    regions.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : regions) {
        buf.putLanguageCode(it.country);
        buf.putBits(it.region_id, 6);
        buf.putBit(1);
        buf.putBit(it.time_offset < 0);
        buf.putMinutesBCD(it.time_offset);
        buf.putMJD(it.next_change, MJD_SIZE);
        buf.putMinutesBCD(it.next_time_offset);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Region region;
        buf.getLanguageCode(region.country);
        buf.getBits(region.region_id, 6);
        buf.skipBits(1);
        const int polarity = buf.getBool() ? -1 : 1;
        region.time_offset = polarity * int(buf.getMinutesBCD());
        region.next_change = buf.getMJD(MJD_SIZE);
        region.next_time_offset = polarity * int(buf.getMinutesBCD());
        regions.push_back(region);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(13)) {
        // Country code is a 3-byte string
        disp << margin << "Country code: " << buf.getLanguageCode() << std::endl;
        disp << margin << UString::Format(u"Region id: %d (0x%<X)", {buf.getBits<uint8_t>(6)});
        buf.skipBits(1);
        const uint8_t polarity = buf.getBit();
        disp << ", polarity: " << (polarity ? "west" : "east") << " of Greenwich" << std::endl;
        disp << margin << UString::Format(u"Local time offset: %s%02d", {polarity ? u"-" : u"", buf.getBCD<uint8_t>(2)});
        disp << UString::Format(u":%02d", {buf.getBCD<uint8_t>(2)}) << std::endl;
        disp << margin << "Next change: " << buf.getMJD(5).format(Time::DATETIME) << std::endl;
        disp << margin << UString::Format(u"Next time offset: %s%02d", {polarity ? u"-" : u"", buf.getBCD<uint8_t>(2)});
        disp << UString::Format(u":%02d", {buf.getBCD<uint8_t>(2)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : regions) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"country_code", it.country);
        e->setIntAttribute(u"country_region_id", it.region_id);
        e->setIntAttribute(u"local_time_offset", it.time_offset);
        e->setDateTimeAttribute(u"time_of_change", it.next_change);
        e->setIntAttribute(u"next_time_offset", it.next_time_offset);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LocalTimeOffsetDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"region");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        Region region;
        ok = children[index]->getAttribute(region.country, u"country_code", true, u"", 3, 3) &&
             children[index]->getIntAttribute<unsigned int>(region.region_id, u"country_region_id", true, 0, 0, 63) &&
             children[index]->getIntAttribute(region.time_offset, u"local_time_offset", true, 0, -780, 780) &&
             children[index]->getDateTimeAttribute(region.next_change, u"time_of_change", true) &&
             children[index]->getIntAttribute(region.next_time_offset, u"next_time_offset", true, 0, -780, 780);
        regions.push_back(region);
    }
    return ok;
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::LocalTimeOffsetDescriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::LocalTimeOffsetDescriptor::merge(const AbstractDescriptor& desc)
{
    const LocalTimeOffsetDescriptor* other = dynamic_cast<const LocalTimeOffsetDescriptor*>(&desc);
    if (other == nullptr) {
        return false;
    }
    else {
        // Loop on all service entries in "other" descriptor.
        for (auto oth = other->regions.begin(); oth != other->regions.end(); ++oth) {
            // Replace entry with same service id in "this" descriptor.
            bool found = false;
            for (auto th = regions.begin(); !found && th != regions.end(); ++th) {
                found = th->country == oth->country && th->region_id == oth->region_id;
                if (found) {
                    *th = *oth;
                }
            }
            // Add service ids which were not found at end of the list.
            if (!found) {
                regions.push_back(*oth);
            }
        }
        // If the result is too large, truncate it.
        bool success = regions.size() <= MAX_REGION;
        while (regions.size() > MAX_REGION) {
            regions.pop_back();
        }
        return success;
    }
}

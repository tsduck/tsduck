//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsLocalTimeOffsetDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"local_time_offset_descriptor"
#define MY_CLASS ts::LocalTimeOffsetDescriptor
#define MY_DID ts::DID_LOCAL_TIME_OFFSET
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    regions()
{
}

ts::LocalTimeOffsetDescriptor::Region::Region() :
    country(),
    region_id(0),
    time_offset(0),
    next_change(),
    next_time_offset(0)
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

void ts::LocalTimeOffsetDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->country)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(uint8_t(it->region_id << 2) | 0x02 | (it->time_offset < 0 ? 0x01 : 0x00));
        bbp->appendUInt8(EncodeBCD(std::abs(it->time_offset) / 60));
        bbp->appendUInt8(EncodeBCD(std::abs(it->time_offset) % 60));
        EncodeMJD(it->next_change, bbp->enlarge(MJD_SIZE), MJD_SIZE);
        bbp->appendUInt8(EncodeBCD(::abs(it->next_time_offset) / 60));
        bbp->appendUInt8(EncodeBCD(::abs(it->next_time_offset) % 60));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() % 13 == 0;
    regions.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 13) {
            Region region;
            region.country = DeserializeLanguageCode(data);
            region.region_id = data[3] >> 2;
            const uint8_t polarity = data[3] & 0x01;
            int hours = DecodeBCD(data[4]);
            int minutes = DecodeBCD(data[5]);
            region.time_offset = (polarity ? -1 : 1) * ((hours * 60) + minutes);
            DecodeMJD(data + 6, 5, region.next_change);
            hours = DecodeBCD(data[11]);
            minutes = DecodeBCD(data[12]);
            region.next_time_offset = (polarity ? -1 : 1) * ((hours * 60) + minutes);

            data += 13;
            size -= 13;

            regions.push_back(region);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 3) {
        // Country code is a 3-byte string
        strm << margin << "Country code: " << DeserializeLanguageCode(data) << std::endl;
        data += 3; size -= 3;
        if (size >= 1) {
            uint8_t region_id = *data >> 2;
            uint8_t polarity = *data & 0x01;
            data += 1; size -= 1;
            strm << margin
                 << UString::Format(u"Region id: %d (0x%X), polarity: %s of Greenwich", {region_id, region_id, polarity ? u"west" : u"east"})
                 << std::endl;
            if (size >= 2) {
                strm << margin
                     << UString::Format(u"Local time offset: %s%02d:%02d", {polarity ? u"-" : u"", DecodeBCD(data[0]), DecodeBCD(data[1])})
                     << std::endl;
                data += 2; size -= 2;
                if (size >= 5) {
                    Time next_change;
                    DecodeMJD(data, 5, next_change);
                    data += 5; size -= 5;
                    strm << margin << "Next change: " << next_change.format(Time::DATETIME) << std::endl;
                    if (size >= 2) {
                        strm << margin
                             << UString::Format(u"Next time offset: %s%02d:%02d", {polarity ? u"-" : u"", DecodeBCD(data[0]), DecodeBCD(data[1])})
                             << std::endl;
                        data += 2; size -= 2;
                    }
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"country_code", it->country);
        e->setIntAttribute(u"country_region_id", it->region_id);
        e->setIntAttribute(u"local_time_offset", it->time_offset);
        e->setDateTimeAttribute(u"time_of_change", it->next_change);
        e->setIntAttribute(u"next_time_offset", it->next_time_offset);
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
             children[index]->getIntAttribute<int>(region.time_offset, u"local_time_offset", true, 0, -780, 780) &&
             children[index]->getDateTimeAttribute(region.next_change, u"time_of_change", true) &&
             children[index]->getIntAttribute<int>(region.next_time_offset, u"next_time_offset", true, 0, -780, 780);
        regions.push_back(region);
    }
    return ok;
}

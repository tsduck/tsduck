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

#include "tsContentAdvisoryDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
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

ts::ContentAdvisoryDescriptor::Entry::Entry() :
    rating_region(0),
    rating_values(),
    rating_description()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(0xC0 | (entries.size() & 0x3F)));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8(it->rating_region);
        bbp->appendUInt8(uint8_t(it->rating_values.size()));
        for (auto it2 = it->rating_values.begin(); it2 != it->rating_values.end(); ++it2) {
            bbp->appendUInt8(it2->first);          // rating_dimension_j
            bbp->appendUInt8(0xF0 | it2->second);  // rating_value
        }
        const size_t len_index = bbp->size();
        bbp->appendUInt8(0x00);  // place-holder for rating_description_length
        const size_t nbytes = it->rating_description.serialize(duck, *bbp, 255, true);
        (*bbp)[len_index] = uint8_t(nbytes);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        size_t reg_count = data[0] & 0x3F;
        data++; size--;
        while (size >= 2 && reg_count > 0) {
            Entry entry;
            entry.rating_region = data[0];
            size_t dim_count = data[1];
            data += 2; size -= 2;
            while (size >= 2 && dim_count > 0) {
                entry.rating_values[data[0]] = data[1] & 0x0F;
                data += 2; size -= 2; dim_count--;
            }
            if (!entry.rating_description.lengthDeserialize(duck, data, size)) {
                _is_valid = false;
                break;
            }
            entries.push_back(entry);
            reg_count--;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t reg_count = data[0] & 0x3F;
        data++; size--;
        strm << margin << "Number of regions: " << reg_count << std::endl;
        while (size >= 2 && reg_count > 0) {
            size_t dim_count = data[1];
            strm << margin << UString::Format(u"- Rating region: 0x%X (%d), number of dimensions: %d", {data[0], data[0], dim_count}) << std::endl;
            data += 2; size -= 2;
            while (size >= 2 && dim_count > 0) {
                strm << margin << UString::Format(u"    Rating dimension j: 0x%X (%d), rating value: %d", {data[0], data[0], data[1] & 0x0F}) << std::endl;
                data += 2; size -= 2; dim_count--;
            }
            if (size >= 1) {
                size_t len = data[0];
                data++; size--;
                ATSCMultipleString::Display(display, u"Rating description: ", indent + 2, data, size, len);
            }
            reg_count--;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentAdvisoryDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"region");
        e->setIntAttribute(u"rating_region", it->rating_region, true);
        for (auto it2 = it->rating_values.begin(); it2 != it->rating_values.end(); ++it2) {
            xml::Element* e2 = e->addElement(u"dimension");
            e2->setIntAttribute(u"rating_dimension_j", it2->first, true);
            e2->setIntAttribute(u"rating_value", it2->second, true);
        }
        it->rating_description.toXML(duck, e, u"rating_description", true);
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
        ok = children[i]->getIntAttribute<uint8_t>(entry.rating_region, u"rating_region", true) &&
             children[i]->getChildren(children2, u"dimension", 0, 255) &&
             entry.rating_description.fromXML(duck, children[i], u"rating_description", false);
        for (size_t i2 = 0; ok && i2 < children2.size(); ++i2) {
            uint8_t dim = 0;
            uint8_t val = 0;
            ok = children2[i2]->getIntAttribute<uint8_t>(dim, u"rating_dimension_j", true) &&
                 children2[i2]->getIntAttribute<uint8_t>(val, u"rating_value", true, 0, 0, 0x0F);
            entry.rating_values[dim] = val;
        }
        entries.push_back(entry);
    }
    return ok;
}

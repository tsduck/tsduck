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
//
//  Representation of a country_availability_descriptor
//
//----------------------------------------------------------------------------

#include "tsCountryAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"country_availability_descriptor"
#define MY_CLASS ts::CountryAvailabilityDescriptor
#define MY_DID ts::DID_COUNTRY_AVAIL
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    country_availability(true),
    country_codes()
{
}

void ts::CountryAvailabilityDescriptor::clearContent()
{
    country_availability = true;
    country_codes.clear();
}

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    country_availability(true),
    country_codes()
{
    deserialize(duck, desc);
}

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(bool availability, const std::initializer_list<UString> countries) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    country_availability(availability),
    country_codes(countries)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(country_availability ? 0xFF : 0x7F);
    for (size_t n = 0; n < country_codes.size(); ++n) {
        if (!SerializeLanguageCode(*bbp, country_codes[n])) {
            desc.invalidate();
            return;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() % 3 == 1;
    country_codes.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        country_availability = (data[0] & 0x80) != 0;
        data++; size--;
        while (size >= 3) {
            country_codes.push_back(DeserializeLanguageCode(data));
            data += 3;
            size -= 3;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        bool available = (data[0] & 0x80) != 0;
        data += 1; size -= 1;
        strm << margin << "Available: " << UString::YesNo(available) << std::endl;
        while (size >= 3) {
            strm << margin << "Country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"country_availability", country_availability);
    for (UStringVector::const_iterator it = country_codes.begin(); it != country_codes.end(); ++it) {
        xml::Element* e = root->addElement(u"country");
        e->setAttribute(u"country_code", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CountryAvailabilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getBoolAttribute(country_availability, u"country_availability", true) &&
        element->getChildren(children, u"country", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        UString name;
        ok = children[i]->getAttribute(name, u"country_code", true, UString(), 3, 3);
        country_codes.push_back(name);
    }
    return ok;
}

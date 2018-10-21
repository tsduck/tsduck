//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"country_availability_descriptor"
#define MY_DID ts::DID_COUNTRY_AVAIL

TS_XML_DESCRIPTOR_FACTORY(ts::CountryAvailabilityDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::CountryAvailabilityDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::CountryAvailabilityDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    country_availability(true),
    country_codes()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    country_availability(true),
    country_codes()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Constructor using a variable-length list of country codes.
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(bool availability, const std::initializer_list<UString> countries) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    country_availability(availability),
    country_codes(countries)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(country_availability ? 0xFF : 0x7F);
    for (size_t n = 0; n < country_codes.size(); ++n) {
        if (!SerializeLanguageCode(*bbp, country_codes[n], charset)) {
            desc.invalidate();
            return;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 3 == 1;
    country_codes.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        country_availability = (data[0] & 0x80) != 0;
        data++; size--;
        while (size >= 3) {
            country_codes.push_back(UString::FromDVB(data, 3, charset));
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
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        bool available = (data[0] & 0x80) != 0;
        data += 1; size -= 1;
        strm << margin << "Available: " << UString::YesNo(available) << std::endl;
        while (size >= 3) {
            strm << margin << "Country code: \"" << UString::FromDVB(data, 3, display.dvbCharset()) << "\"" << std::endl;
            data += 3; size -= 3;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::buildXML(xml::Element* root) const
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

void ts::CountryAvailabilityDescriptor::fromXML(const xml::Element* element)
{
    country_codes.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getBoolAttribute(country_availability, u"country_availability", true) &&
        element->getChildren(children, u"country", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        UString name;
        _is_valid = children[i]->getAttribute(name, u"country_code", true, UString(), 3, 3);
        if (_is_valid) {
            country_codes.push_back(name);
        }
    }
}

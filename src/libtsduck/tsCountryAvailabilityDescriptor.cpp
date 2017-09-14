//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::CountryAvailabilityDescriptor, "country_availability_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::CountryAvailabilityDescriptor, ts::EDID(ts::DID_COUNTRY_AVAIL));
TS_ID_DESCRIPTOR_DISPLAY(ts::CountryAvailabilityDescriptor::DisplayDescriptor, ts::EDID(ts::DID_COUNTRY_AVAIL));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor() :
    AbstractDescriptor(DID_COUNTRY_AVAIL, "country_availability_descriptor"),
    country_availability(true),
    country_codes()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_COUNTRY_AVAIL, "country_availability_descriptor"),
    country_availability(true),
    country_codes()
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Constructor using a variable-length argument list.
// Each argument is a country_code
// The end of the argument list must be marked by TS_NULL.
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(bool availability, const char* country, ...) :
    AbstractDescriptor(DID_COUNTRY_AVAIL, "country_availability_descriptor"),
    country_availability(availability),
    country_codes()
{
    _is_valid = true;
    if (country != TS_NULL) {
        country_codes.push_back(country);
        va_list ap;
        va_start(ap, country);
        while ((country = va_arg(ap, const char*)) != TS_NULL) {
            country_codes.push_back(country);
        }
        va_end(ap);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::serialize (Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (3));
    CheckNonNull (bbp.pointer());

    for (size_t n = 0; n < country_codes.size(); ++n) {
        if (country_codes[n].size() != 3) {
            desc.invalidate();
            return;
        }
        bbp->append (country_codes[n]);
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    (*bbp)[2] = country_availability ? 0xFF : 0x7F;
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 3 == 1;
    country_codes.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        country_availability = (data[0] & 0x80) != 0;
        data++; size--;
        while (size >= 3) {
            country_codes.push_back (std::string (reinterpret_cast <const char*> (data), 3));
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
        strm << margin << "Available: " << YesNo(available) << std::endl;
        while (size >= 3) {
            strm << margin << "Country code: \"" << Printable(data, 3) << "\"" << std::endl;
            data += 3; size -= 3;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::CountryAvailabilityDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setBoolAttribute(root, "country_availability", country_availability);
    for (StringVector::const_iterator it = country_codes.begin(); it != country_codes.end(); ++it) {
        XML::Element* e = xml.addElement(root, "country");
        xml.setAttribute(e, "country_code", *it);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    country_codes.clear();

    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getBoolAttribute(country_availability, element, "country_availability", true) &&
        xml.getChildren(children, element, "country", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        std::string name;
        _is_valid = xml.getAttribute(name, children[i], "country_code", true, "", 3, 3);
        if (_is_valid) {
            country_codes.push_back(name);
        }
    }
}

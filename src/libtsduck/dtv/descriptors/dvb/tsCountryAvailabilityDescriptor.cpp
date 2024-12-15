//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCountryAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"country_availability_descriptor"
#define MY_CLASS    ts::CountryAvailabilityDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_COUNTRY_AVAIL, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::CountryAvailabilityDescriptor::clearContent()
{
    country_availability = true;
    country_codes.clear();
}

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    CountryAvailabilityDescriptor()
{
    deserialize(duck, desc);
}

ts::CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(bool availability, const std::initializer_list<UString> countries) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    country_availability(availability),
    country_codes(countries)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(country_availability);
    buf.putBits(0xFF, 7);
    for (const auto& code : country_codes) {
        buf.putLanguageCode(code);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::deserializePayload(PSIBuffer& buf)
{
    country_availability = buf.getBool();
    buf.skipBits(7);
    while (buf.canRead()) {
        country_codes.push_back(buf.getLanguageCode());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Available: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipBits(7);
        while (buf.canReadBytes(3)) {
            disp << margin << "Country code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CountryAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"country_availability", country_availability);
    for (const auto& code : country_codes) {
        xml::Element* e = root->addElement(u"country");
        e->setAttribute(u"country_code", code);
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

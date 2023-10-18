//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsParentalRatingDescriptor.h"
#include "tsDescriptor.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"parental_rating_descriptor"
#define MY_CLASS ts::ParentalRatingDescriptor
#define MY_DID ts::DID_PARENTAL_RATING
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ParentalRatingDescriptor::Entry::Entry(const UChar* code, uint8_t rate) :
    country_code(code),
    rating(rate)
{
}

ts::ParentalRatingDescriptor::Entry::Entry(const UString& code, uint8_t rate) :
    country_code(code),
    rating(rate)
{
}

ts::ParentalRatingDescriptor::ParentalRatingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ParentalRatingDescriptor::clearContent()
{
    entries.clear();
}

ts::ParentalRatingDescriptor::ParentalRatingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ParentalRatingDescriptor()
{
    deserialize(duck, desc);
}

ts::ParentalRatingDescriptor::ParentalRatingDescriptor(const UString& code, uint8_t rate) :
    ParentalRatingDescriptor()
{
    entries.push_back(Entry(code, rate));
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putLanguageCode(it.country_code);
        buf.putUInt8(it.rating);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        buf.getLanguageCode(e.country_code);
        e.rating = buf.getUInt8();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << "Country code: " << buf.getLanguageCode();
        const uint8_t rating = buf.getUInt8();
        disp << UString::Format(u", rating: 0x%X ", {rating});
        if (rating == 0) {
            disp << "(undefined)";
        }
        else if (rating <= 0x0F) {
            disp << "(min. " << int(rating + 3) << " years)";
        }
        else {
            disp << "(broadcaster-defined)";
        }
        disp << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"country");
        e->setAttribute(u"country_code", it.country_code);
        e->setIntAttribute(u"rating", it.rating, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ParentalRatingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"country", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.country_code, u"country_code", true, u"", 3, 3) &&
             children[i]->getIntAttribute(entry.rating, u"rating", true, 0, 0x00, 0xFF);
        entries.push_back(entry);
    }
    return ok;
}

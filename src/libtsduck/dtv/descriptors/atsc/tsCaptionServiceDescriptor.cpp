//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCaptionServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"caption_service_descriptor"
#define MY_CLASS ts::CaptionServiceDescriptor
#define MY_DID ts::DID_ATSC_CAPTION
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CaptionServiceDescriptor::CaptionServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CaptionServiceDescriptor::clearContent()
{
    entries.clear();
}

ts::CaptionServiceDescriptor::CaptionServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    CaptionServiceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 3);
    buf.putBits(entries.size(), 5);
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language);
        buf.putBit(it.digital_cc);
        buf.putBit(1);
        if (it.digital_cc) {
            buf.putBits(it.caption_service_number, 6);
        }
        else {
            buf.putBits(0xFF, 5);
            buf.putBit(it.line21_field);
        }
        buf.putBit(it.easy_reader);
        buf.putBit(it.wide_aspect_ratio);
        buf.putBits(0xFFFF, 14);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(3);
    const size_t count = buf.getBits<size_t>(5);
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Entry e;
        buf.getLanguageCode(e.language);
        e.digital_cc = buf.getBool();
        buf.skipBits(1);
        if (e.digital_cc) {
            buf.getBits(e.caption_service_number, 6);
        }
        else {
            buf.skipBits(5);
            e.line21_field = buf.getBool();
        }
        e.easy_reader = buf.getBool();
        e.wide_aspect_ratio = buf.getBool();
        buf.skipBits(14);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(3);
        const size_t count = buf.getBits<size_t>(5);
        disp << margin << "Number of services: " << count << std::endl;
        for (size_t i = 0; i < count && buf.canReadBytes(6); ++i) {
            disp << margin << "- Language: \"" << buf.getLanguageCode() << "\"";
            const bool digital = buf.getBool();
            buf.skipBits(1);
            disp << UString::Format(u", digital: %s", {digital});
            if (digital) {
                disp << UString::Format(u", service: 0x%X (%<d)", {buf.getBits<uint8_t>(6)});
            }
            else {
                buf.skipBits(5);
                disp << UString::Format(u", line 21: %s", {buf.getBool()});
            }
            disp << UString::Format(u", easy reader: %s", {buf.getBool()});
            disp << UString::Format(u", wide: %s", {buf.getBool()}) << std::endl;
            buf.skipBits(14);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setAttribute(u"language", it.language);
        e->setBoolAttribute(u"digital_cc", it.digital_cc);
        if (it.digital_cc) {
            e->setIntAttribute(u"caption_service_number", it.caption_service_number, true);
        }
        else {
            e->setBoolAttribute(u"line21_field", it.line21_field);
        }
        e->setBoolAttribute(u"easy_reader", it.easy_reader);
        e->setBoolAttribute(u"wide_aspect_ratio", it.wide_aspect_ratio);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CaptionServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language, u"language", true, UString(), 3, 3) &&
             children[i]->getBoolAttribute(entry.digital_cc, u"digital_cc", true) &&
             children[i]->getBoolAttribute(entry.line21_field, u"line21_field", false) &&
             children[i]->getIntAttribute(entry.caption_service_number, u"caption_service_number", false, 0, 0, 0x3F) &&
             children[i]->getBoolAttribute(entry.easy_reader, u"easy_reader", true) &&
             children[i]->getBoolAttribute(entry.wide_aspect_ratio, u"wide_aspect_ratio", true);
        entries.push_back(entry);
    }
    return ok;
}

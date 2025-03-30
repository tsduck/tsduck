//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCComponentListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_component_list_descriptor"
#define MY_CLASS    ts::ATSCComponentListDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_COMPONENT_LIST, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCComponentListDescriptor::ATSCComponentListDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCComponentListDescriptor::clearContent()
{
    alternate = false;
    components.clear();
}

ts::ATSCComponentListDescriptor::ATSCComponentListDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCComponentListDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCComponentListDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(alternate);
    buf.putBits(components.size(), 7);
    for (const auto& cp : components) {
        buf.putUInt8(cp.stream_type);
        buf.putUInt32(cp.format_identifier);
        buf.putUInt8(uint8_t(cp.stream_info_details.size()));
        buf.putBytes(cp.stream_info_details);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCComponentListDescriptor::deserializePayload(PSIBuffer& buf)
{
    alternate = buf.getBool();
    const size_t count = buf.getBits<size_t>(7);
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Component cp;
        cp.stream_type = buf.getUInt8();
        cp.format_identifier = buf.getUInt32();
        buf.getBytes(cp.stream_info_details, buf.getUInt8());
        components.push_back(cp);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCComponentListDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canRead()) {
        disp << margin << "Alternate: " << UString::YesNo(buf.getBool()) << std::endl;
        const size_t count = buf.getBits<size_t>(7);
        disp << margin << "Number of components: " << count << std::endl;
        for (size_t i = 0; i < count && buf.canReadBytes(6); ++i) {
            disp << margin << UString::Format(u"- Stream type: %n", buf.getUInt8());
            disp << UString::Format(u", format identifier: %n", buf.getUInt32()) << std::endl;
            disp.displayPrivateData(u"Stream info", buf, buf.getUInt8(), margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCComponentListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"alternate", alternate);
    for (const auto& cp : components) {
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"stream_type", cp.stream_type, true);
        e->setIntAttribute(u"format_identifier", cp.format_identifier, true);
        e->addHexaTextChild(u"stream_info", cp.stream_info_details, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCComponentListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok = element->getBoolAttribute(alternate, u"alternate", true) &&
              element->getChildren(xcomp, u"component");

    for (size_t i = 0; ok && i < xcomp.size(); ++i) {
        Component cp;
        ok = xcomp[i]->getIntAttribute(cp.stream_type, u"stream_type", true) &&
             xcomp[i]->getIntAttribute(cp.format_identifier, u"format_identifier", true) &&
             xcomp[i]->getHexaTextChild(cp.stream_info_details, u"stream_info");
        components.push_back(cp);
    }
    return ok;
}

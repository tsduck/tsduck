//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023-2026, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRNTScanDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RNT_scan_descriptor"
#define MY_CLASS    ts::RNTScanDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_RNT_SCAN, ts::Standards::DVB, ts::TID_RNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RNTScanDescriptor::RNTScanDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::RNTScanDescriptor::RNTScanDescriptor(DuckContext& duck, const Descriptor& desc) :
    RNTScanDescriptor()
{
    deserialize(duck, desc);
}

ts::RNTScanDescriptor::ScanTriplet::ScanTriplet()
{
    clearContent();
}


void ts::RNTScanDescriptor::clearContent()
{
    RNTreferences.clear();
}

void ts::RNTScanDescriptor::ScanTriplet::clearContent()
{
    transport_stream_id = 0;
    original_network_id = 0;
    scan_weighting = 0;
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RNTScanDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto ref : RNTreferences) {
        ref.serialize(buf);
    }
}

void ts::RNTScanDescriptor::ScanTriplet::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUInt8(scan_weighting);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RNTScanDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canReadBytes(5)) {
        ScanTriplet newRef(buf);
        RNTreferences.push_back(newRef);
    }
}

void ts::RNTScanDescriptor::ScanTriplet::deserialize(PSIBuffer& buf)
{
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    scan_weighting = buf.getUInt8();
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RNTScanDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(5)) {
        ScanTriplet reference;
        reference.display(disp, buf, margin);
    }
}

void ts::RNTScanDescriptor::ScanTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Transport stream: 0x%X", buf.getUInt16());
    disp << "" << UString::Format(u", original network: 0x%X", buf.getUInt16());
    disp << ", weighting: " << int(buf.getUInt8()) << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RNTScanDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto ref : RNTreferences) {
        ref.toXML(root->addElement(u"RNT_reference"));
    }
}

void ts::RNTScanDescriptor::ScanTriplet::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"scan_weighting", scan_weighting);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RNTScanDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& child : element->children(u"RNT_reference", &ok, 1)) {
        RNTreferences.emplace_back().fromXML(&child);
    }
    return ok;
}

bool ts::RNTScanDescriptor::ScanTriplet::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
           element->getIntAttribute(original_network_id, u"original_network_id", true) &&
           element->getIntAttribute(scan_weighting, u"scan_weighting", true);
}

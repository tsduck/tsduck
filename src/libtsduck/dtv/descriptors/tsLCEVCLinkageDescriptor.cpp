//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLCEVCLinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"LCEVC_linkage_descriptor"
#define MY_CLASS ts::LCEVCLinkageDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_LCEVC_LINKAGE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LCEVCLinkageDescriptor::LCEVCLinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::LCEVCLinkageDescriptor::clearContent()
{
    lcevc_stream_tags.clear();

}

ts::LCEVCLinkageDescriptor::LCEVCLinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    LCEVCLinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::LCEVCLinkageDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    uint8_t num_lcevc_stream_tags = lcevc_stream_tags.size() & 0xFF;
    buf.putUInt8(num_lcevc_stream_tags);
    for (auto it : lcevc_stream_tags) {
        buf.putUInt8(it);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    uint8_t num_lcevc_stream_tags = buf.getUInt8();
    for (uint8_t i = 0; i < num_lcevc_stream_tags; i++) {
        lcevc_stream_tags.push_back(buf.getUInt8());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        size_t num_lcevc_stream_tags = buf.getUInt8();
        std::vector<uint8_t> lcevc_stream_tag;
        for (uint8_t i = 0; i < num_lcevc_stream_tags; i++) {
            lcevc_stream_tag.push_back(buf.getUInt8());
        }
        disp.displayVector(u"LCEVC stream tag:", lcevc_stream_tag, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaTextChild(u"lcevc_stream_tag", lcevc_stream_tags, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LCEVCLinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(lcevc_stream_tags, u"lcevc_stream_tag", false, 0, MAX_DESCRIPTOR_SIZE - 1);
}

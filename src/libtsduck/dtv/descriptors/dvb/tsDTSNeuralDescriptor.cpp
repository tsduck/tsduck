//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTSNeuralDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DTS_neural_descriptor"
#define MY_CLASS    ts::DTSNeuralDescriptor
#define MY_EDID     ts::EDID::ExtensionDVB(ts::XDID_DVB_DTS_NEURAL)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DTSNeuralDescriptor::DTSNeuralDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DTSNeuralDescriptor::DTSNeuralDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTSNeuralDescriptor()
{
    deserialize(duck, desc);
}

void ts::DTSNeuralDescriptor::clearContent()
{
    config_id = 0;
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTSNeuralDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(config_id);
    buf.putBytes(additional_info);
}

void ts::DTSNeuralDescriptor::deserializePayload(PSIBuffer& buf)
{
    config_id = buf.getUInt8();
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTSNeuralDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Config Id: %n)", buf.getUInt8()) << std::endl;
        disp.displayPrivateData(u"Additional info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTSNeuralDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"config_id", config_id, true);
    root->addHexaTextChild(u"additional_info", additional_info, true);
}

bool ts::DTSNeuralDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(config_id, u"config_id", true) &&
           element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}

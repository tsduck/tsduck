//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsXAITPIDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"xait_pid_descriptor"
#define MY_CLASS    ts::XAITPIDDescriptor
#define MY_EDID     ts::EDID::ExtensionDVB(ts::XDID_DVB_XAIT_PID)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::XAITPIDDescriptor::XAITPIDDescriptor(PID pid) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    xait_PID(pid)
{
}

void ts::XAITPIDDescriptor::clearContent()
{
    xait_PID = PID_NULL;
}

ts::XAITPIDDescriptor::XAITPIDDescriptor(DuckContext& duck, const Descriptor& desc) :
    XAITPIDDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::XAITPIDDescriptor::serializePayload(PSIBuffer& buf) const
{
    // The spec says that the PID is written on 16 bits instead of the usual 13 bits + 3 reserved '1' bits.
    buf.putUInt16(xait_PID);
}

void ts::XAITPIDDescriptor::deserializePayload(PSIBuffer& buf)
{
    // Eliminate the 3 msb in case the signalization is incorrect and set them as '1'.
    xait_PID = buf.getUInt16() & 0x1FFF;
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::XAITPIDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"xait_PID", xait_PID, true);
}

bool ts::XAITPIDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(xait_PID, u"xait_PID", true, 0, 0x0000, 0x1FFF);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::XAITPIDDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"XAIT PID: %n", buf.getUInt16()) << std::endl;
    }
}

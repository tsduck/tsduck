//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCPIDCountDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_PID_count_descriptor"
#define MY_CLASS    ts::ATSCPIDCountDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_PID_COUNT, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCPIDCountDescriptor::ATSCPIDCountDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCPIDCountDescriptor::clearContent()
{
    total_number_of_PIDs = 0;
    min_number_of_PIDs = 0;
}

ts::ATSCPIDCountDescriptor::ATSCPIDCountDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCPIDCountDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCPIDCountDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putPID(total_number_of_PIDs);
    buf.putPID(min_number_of_PIDs);
}

void ts::ATSCPIDCountDescriptor::deserializePayload(PSIBuffer& buf)
{
    total_number_of_PIDs = buf.getPID();
    min_number_of_PIDs = buf.getPID();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCPIDCountDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Total number of PID's: " << buf.getPID() << std::endl;
        disp << margin << "Minimum number of PID's: " << buf.getPID() << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCPIDCountDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"total_number_of_PIDs", total_number_of_PIDs);
    root->setIntAttribute(u"min_number_of_PIDs", min_number_of_PIDs);
}

bool ts::ATSCPIDCountDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(total_number_of_PIDs, u"total_number_of_PIDs", false, 0, 0, 0x1FFF) &&
           element->getIntAttribute(min_number_of_PIDs, u"min_number_of_PIDs", false, 0, 0, 0x1FFF);
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCPDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CP_descriptor"
#define MY_CLASS    ts::CPDescriptor
#define MY_EDID     ts::EDID::ExtensionDVB(ts::XDID_DVB_CP)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::CPDescriptor::CPDescriptor(uint16_t cp_id_, PID cp_pid_) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    cp_id(cp_id_),
    cp_pid(cp_pid_),
    private_data()
{
}

void ts::CPDescriptor::clearContent()
{
    cp_id = 0;
    cp_pid = PID_NULL;
    private_data.clear();
}

ts::CPDescriptor::CPDescriptor(DuckContext& duck, const Descriptor& desc) :
    CPDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::CPDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(cp_id);
    buf.putPID(cp_pid);
    buf.putBytes(private_data);
}

void ts::CPDescriptor::deserializePayload(PSIBuffer& buf)
{
    cp_id = buf.getUInt16();
    cp_pid = buf.getPID();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::CPDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CP_system_id", cp_id, true);
    root->setIntAttribute(u"CP_PID", cp_pid, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::CPDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(cp_id, u"CP_system_id", true, 0, 0x0000, 0xFFFF) &&
           element->getIntAttribute<PID>(cp_pid, u"CP_PID", true, 0, 0x0000, 0x1FFF) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CPDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "CP System Id: " << DataName(MY_XML_NAME, u"CPSystemId", buf.getUInt16(), NamesFlags::FIRST);
        disp << UString::Format(u", CP PID: %n", buf.getPID()) << std::endl;
        disp.displayPrivateData(u"Private CP data", buf, NPOS, margin);
    }
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPSignallingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ip_signalling_descriptor"
#define MY_CLASS ts::IPSignallingDescriptor
#define MY_DID ts::DID_AIT_IP_SIGNALLING
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IPSignallingDescriptor::IPSignallingDescriptor(uint32_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    platform_id(id)
{
}

ts::IPSignallingDescriptor::IPSignallingDescriptor(DuckContext& duck, const Descriptor& desc) :
    IPSignallingDescriptor()
{
    deserialize(duck, desc);
}

void ts::IPSignallingDescriptor::clearContent()
{
    platform_id = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IPSignallingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt24(platform_id);
}

void ts::IPSignallingDescriptor::deserializePayload(PSIBuffer& buf)
{
    platform_id = buf.getUInt24();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IPSignallingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Platform id: " << DataName(u"INT", u"platform_id", buf.getUInt24(), NamesFlags::FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IPSignallingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"platform_id", platform_id, true);
}

bool ts::IPSignallingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(platform_id, u"platform_id", true, 0, 0, 0x00FFFFFF);
}

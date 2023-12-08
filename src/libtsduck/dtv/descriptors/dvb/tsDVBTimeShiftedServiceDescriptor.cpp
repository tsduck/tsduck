//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBTimeShiftedServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DVB_time_shifted_service_descriptor"
#define MY_XML_NAME_LEGACY u"time_shifted_service_descriptor"
#define MY_CLASS ts::DVBTimeShiftedServiceDescriptor
#define MY_DID ts::DID_TIME_SHIFT_SERVICE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBTimeShiftedServiceDescriptor::DVBTimeShiftedServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY)
{
}

ts::DVBTimeShiftedServiceDescriptor::DVBTimeShiftedServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBTimeShiftedServiceDescriptor()
{
    deserialize(duck, desc);
}

void ts::DVBTimeShiftedServiceDescriptor::clearContent()
{
    reference_service_id = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBTimeShiftedServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(reference_service_id);
}

void ts::DVBTimeShiftedServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    reference_service_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBTimeShiftedServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Reference service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBTimeShiftedServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"reference_service_id", reference_service_id, true);
}

bool ts::DVBTimeShiftedServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(reference_service_id, u"reference_service_id", true);
}

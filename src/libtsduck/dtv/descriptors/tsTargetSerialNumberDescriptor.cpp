//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetSerialNumberDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_serial_number_descriptor"
#define MY_CLASS ts::TargetSerialNumberDescriptor
#define MY_DID ts::DID_INT_SERIAL_NUM
#define MY_STD ts::Standards::DVB

// Table-specific descriptor which is allowed in two distinct tables.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_INT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetSerialNumberDescriptor::TargetSerialNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TargetSerialNumberDescriptor::clearContent()
{
    serial_data.clear();
}

ts::TargetSerialNumberDescriptor::TargetSerialNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetSerialNumberDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetSerialNumberDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(serial_data);
}

void ts::TargetSerialNumberDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(serial_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetSerialNumberDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp.displayPrivateData(u"Serial number", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetSerialNumberDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaText(serial_data, true);
}

bool ts::TargetSerialNumberDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaText(serial_data, 0, MAX_DESCRIPTOR_SIZE - 2);
}

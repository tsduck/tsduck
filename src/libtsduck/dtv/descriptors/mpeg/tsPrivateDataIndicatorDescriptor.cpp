//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPrivateDataIndicatorDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"private_data_indicator_descriptor"
#define MY_CLASS ts::PrivateDataIndicatorDescriptor
#define MY_DID ts::DID_PRIV_DATA_IND
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PrivateDataIndicatorDescriptor::PrivateDataIndicatorDescriptor(uint32_t pdi) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    private_data_indicator(pdi)
{
}

ts::PrivateDataIndicatorDescriptor::PrivateDataIndicatorDescriptor(DuckContext& duck, const Descriptor& desc) :
    PrivateDataIndicatorDescriptor(0)
{
    deserialize(duck, desc);
}

void ts::PrivateDataIndicatorDescriptor::clearContent()
{
    private_data_indicator = 0;
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::PrivateDataIndicatorDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(private_data_indicator);
}

void ts::PrivateDataIndicatorDescriptor::deserializePayload(PSIBuffer& buf)
{
    private_data_indicator = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PrivateDataIndicatorDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        // Sometimes, the indicator is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Private data indicator: 0x%08X", buf, 4, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::PrivateDataIndicatorDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"private_data_indicator", private_data_indicator, true);
}

bool ts::PrivateDataIndicatorDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(private_data_indicator, u"private_data_indicator", true);
}

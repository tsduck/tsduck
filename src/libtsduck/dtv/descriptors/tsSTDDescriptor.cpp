//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSTDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"STD_descriptor"
#define MY_CLASS ts::STDDescriptor
#define MY_DID ts::DID_STD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::STDDescriptor::STDDescriptor(bool leak_valid_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    leak_valid(leak_valid_)
{
}

ts::STDDescriptor::STDDescriptor(DuckContext& duck, const Descriptor& desc) :
    STDDescriptor()
{
    deserialize(duck, desc);
}

void ts::STDDescriptor::clearContent()
{
    leak_valid = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::STDDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 7);
    buf.putBit(leak_valid);
}

void ts::STDDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(7);
    leak_valid = buf.getBool();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::STDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(7);
        const bool leak = buf.getBool();
        disp << margin << "Link valid flag: " << int(leak) << (leak ? " (leak)" : " (vbv_delay)") << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::STDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"leak_valid", leak_valid);
}

bool ts::STDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(leak_valid, u"leak_valid", true);
}

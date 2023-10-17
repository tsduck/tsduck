//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCopyrightDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"copyright_descriptor"
#define MY_CLASS ts::CopyrightDescriptor
#define MY_DID ts::DID_COPYRIGHT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CopyrightDescriptor::CopyrightDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CopyrightDescriptor::clearContent()
{
    copyright_identifier = 0;
    additional_copyright_info.clear();
}

ts::CopyrightDescriptor::CopyrightDescriptor(DuckContext& duck, const Descriptor& desc) :
    CopyrightDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::CopyrightDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(copyright_identifier);
    buf.putBytes(additional_copyright_info);
}

void ts::CopyrightDescriptor::deserializePayload(PSIBuffer& buf)
{
    copyright_identifier = buf.getUInt32();
    buf.getBytes(additional_copyright_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CopyrightDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        // Sometimes, the copyright identifier is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Copyright identifier: 0x%08X", buf, 4, margin);
        disp.displayPrivateData(u"Additional copyright info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::CopyrightDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"copyright_identifier", copyright_identifier, true);
    root->addHexaTextChild(u"additional_copyright_info", additional_copyright_info, true);
}

bool ts::CopyrightDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(copyright_identifier, u"copyright_identifier", true) &&
           element->getHexaTextChild(additional_copyright_info, u"additional_copyright_info", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}

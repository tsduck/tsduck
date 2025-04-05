//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBScramblerDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_scrambler_descriptor"
#define MY_CLASS    ts::ISDBScramblerDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_SCRAMBLER, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBScramblerDescriptor::ISDBScramblerDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBScramblerDescriptor::ISDBScramblerDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBScramblerDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBScramblerDescriptor::clearContent()
{
    scrambler_identification = 0;
    data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBScramblerDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(scrambler_identification);
    buf.putBytes(data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBScramblerDescriptor::deserializePayload(PSIBuffer& buf)
{
    scrambler_identification = buf.getUInt8();
    buf.getBytes(data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBScramblerDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canRead()) {
        disp << margin << UString::Format(u"Scrambler identification: %n", buf.getUInt8()) << std::endl;
        disp.displayPrivateData(u"Data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBScramblerDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"scrambler_identification", scrambler_identification, true);
    root->addHexaTextChild(u"data", data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBScramblerDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(scrambler_identification, u"scrambler_identification", true) &&
           element->getHexaTextChild(data, u"data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioCommandDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_command_descriptor"
#define MY_CLASS ts::MPEGH3DAudioCommandDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::EDID_MPEG_MPH3D_COMMAND
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioCommandDescriptor::MPEGH3DAudioCommandDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEGH3DAudioCommandDescriptor::clearContent()
{
    MHAS.clear();
}

ts::MPEGH3DAudioCommandDescriptor::MPEGH3DAudioCommandDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioCommandDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioCommandDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioCommandDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(MHAS);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioCommandDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(MHAS);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioCommandDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp.displayPrivateData(u"MHAS", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioCommandDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaTextChild(u"MHAS", MHAS, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioCommandDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(MHAS, u"MHAS", false, 0, MAX_DESCRIPTOR_SIZE);
}

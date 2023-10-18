//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG4AudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEG4_audio_descriptor"
#define MY_CLASS ts::MPEG4AudioDescriptor
#define MY_DID ts::DID_MPEG4_AUDIO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG4AudioDescriptor::MPEG4AudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::MPEG4AudioDescriptor::MPEG4AudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEG4AudioDescriptor()
{
    deserialize(duck, desc);
}

void ts::MPEG4AudioDescriptor::clearContent()
{
    MPEG4_audio_profile_and_level = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG4AudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(MPEG4_audio_profile_and_level);
}

void ts::MPEG4AudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    MPEG4_audio_profile_and_level = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEG4AudioDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"MPEG-4 Audio profile and level: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG4AudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"MPEG4_audio_profile_and_level", MPEG4_audio_profile_and_level, true);
}

bool ts::MPEG4AudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(MPEG4_audio_profile_and_level, u"MPEG4_audio_profile_and_level", true);
}

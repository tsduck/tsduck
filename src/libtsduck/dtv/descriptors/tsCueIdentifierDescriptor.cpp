//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCueIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"cue_identifier_descriptor"
#define MY_CLASS ts::CueIdentifierDescriptor
#define MY_DID ts::DID_CUE_IDENTIFIER
#define MY_STD ts::Standards::SCTE

// This is a non-DVB descriptor with DID >= 0x80 => must set PDS to zero in EDID.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, 0), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Definition of names for cue stream types.
//----------------------------------------------------------------------------

const ts::Enumeration ts::CueIdentifierDescriptor::CueStreamTypeNames({
    {u"insert_null_schedule", 0x00},
    {u"all",                  0x01},
    {u"segmentation",         0x02},
    {u"tiered_splicing",      0x03},
    {u"tiered_segmentation",  0x04},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CueIdentifierDescriptor::CueIdentifierDescriptor(uint8_t type) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cue_stream_type(type)
{
}

void ts::CueIdentifierDescriptor::clearContent()
{
    cue_stream_type = CUE_ALL_COMMANDS;
}

ts::CueIdentifierDescriptor::CueIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    CueIdentifierDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CueIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(cue_stream_type);
}

void ts::CueIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    cue_stream_type = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CueIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        const uint8_t type = buf.getUInt8();
        disp << margin << UString::Format(u"Cue stream type: 0x%X", {type});
        switch (type) {
            case 0x00: disp << " (splice_insert, splice_null, splice_schedule)"; break;
            case 0x01: disp << " (All commands)"; break;
            case 0x02: disp << " (Segmentation)"; break;
            case 0x03: disp << " (Tiered splicing)"; break;
            case 0x04: disp << " (Tiered segmentation)"; break;
            default: break;
        }
        disp << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CueIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setEnumAttribute(CueStreamTypeNames, u"cue_stream_type", cue_stream_type);
}

bool ts::CueIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntEnumAttribute(cue_stream_type, CueStreamTypeNames, u"cue_stream_type", true);
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDataStreamAlignmentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"data_stream_alignment_descriptor"
#define MY_CLASS ts::DataStreamAlignmentDescriptor
#define MY_DID ts::DID_DATA_ALIGN
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DataStreamAlignmentDescriptor::DataStreamAlignmentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DataStreamAlignmentDescriptor::DataStreamAlignmentDescriptor(DuckContext& duck, const Descriptor& desc) :
    DataStreamAlignmentDescriptor()
{
    deserialize(duck, desc);
}

void ts::DataStreamAlignmentDescriptor::clearContent()
{
    alignment_type = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataStreamAlignmentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(alignment_type);
}

void ts::DataStreamAlignmentDescriptor::deserializePayload(PSIBuffer& buf)
{
    alignment_type = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataStreamAlignmentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Alignment type: " << DataName(MY_XML_NAME, u"DataStreamAlignment", buf.getUInt8(), NamesFlags::BOTH_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML (de)serialization
//----------------------------------------------------------------------------

void ts::DataStreamAlignmentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"alignment_type", alignment_type, true);
}

bool ts::DataStreamAlignmentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(alignment_type, u"alignment_type", true);
}

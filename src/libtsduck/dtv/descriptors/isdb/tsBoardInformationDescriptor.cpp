//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBoardInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"board_information_descriptor"
#define MY_CLASS ts::BoardInformationDescriptor
#define MY_DID ts::DID_ISDB_BOARD_INFO
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BoardInformationDescriptor::BoardInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::BoardInformationDescriptor::BoardInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    BoardInformationDescriptor()
{
    deserialize(duck, desc);
}

void ts::BoardInformationDescriptor::clearContent()
{
    title.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::BoardInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(title);
    buf.putStringWithByteLength(text);
}

void ts::BoardInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getStringWithByteLength(title);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::BoardInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Title: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::BoardInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"title", title);
    root->setAttribute(u"text", text);
}

bool ts::BoardInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(title, u"title") && element->getAttribute(text, u"text");
}

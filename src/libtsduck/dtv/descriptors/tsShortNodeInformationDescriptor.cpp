//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsShortNodeInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"short_node_information_descriptor"
#define MY_CLASS ts::ShortNodeInformationDescriptor
#define MY_DID ts::DID_ISDB_SHORT_NODE_INF
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ShortNodeInformationDescriptor::ShortNodeInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ShortNodeInformationDescriptor::ShortNodeInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ShortNodeInformationDescriptor()
{
    deserialize(duck, desc);
}

void ts::ShortNodeInformationDescriptor::clearContent()
{
    ISO_639_language_code.clear();
    node_name.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ShortNodeInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(node_name);
    buf.putStringWithByteLength(text);
}

void ts::ShortNodeInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(node_name);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortNodeInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        disp << margin << "Node name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ShortNodeInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->setAttribute(u"node_name", node_name, true);
    root->setAttribute(u"text", text, true);
}

bool ts::ShortNodeInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, UString(), 3, 3) &&
           element->getAttribute(node_name, u"node_name", false) &&
           element->getAttribute(text, u"text", false);
}

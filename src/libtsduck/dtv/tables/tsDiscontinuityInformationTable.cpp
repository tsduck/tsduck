//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsDiscontinuityInformationTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"discontinuity_information_table"
#define MY_CLASS ts::DiscontinuityInformationTable
#define MY_TID ts::TID_DIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DiscontinuityInformationTable::DiscontinuityInformationTable(bool tr) :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    transition(tr)
{
}

ts::DiscontinuityInformationTable::DiscontinuityInformationTable(DuckContext& duck, const BinaryTable& table) :
    DiscontinuityInformationTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::clearContent()
{
    transition = false;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Reference to single section
    const Section& sect(*table.sectionAt(0));

    // Get content
    if (sect.payloadSize() >= 1) {
        transition = (sect.payload()[0] & 0x80) != 0;
        _is_valid = true;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Encode the data in the payload
    const uint8_t payload = transition ? 0xFF : 0x7F;

    // Add the section in the table
    table.addSection(new Section(MY_TID, true, &payload, 1));
}


//----------------------------------------------------------------------------
// A static method to display a DiscontinuityInformationTable section.
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 1) {
        strm << UString::Format(u"%*sTransition: %s", {indent, u"", UString::YesNo((data[0] & 0x80) != 0)}) << std::endl;
        data++; size--;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"transition", transition);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DiscontinuityInformationTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(transition, u"transition", true);
}

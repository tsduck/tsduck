//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsDSMCCStreamDescriptorsTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSMCC_stream_descriptors_table"
#define MY_CLASS ts::DSMCCStreamDescriptorsTable
#define MY_TID ts::TID_DSMCC_SD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCStreamDescriptorsTable::DSMCCStreamDescriptorsTable(uint8_t vers, bool cur, uint16_t tid_ext) :
    AbstractDescriptorsTable(MY_TID, MY_XML_NAME, MY_STD, tid_ext, vers, cur),
    table_id_extension(_tid_ext)
{
}

ts::DSMCCStreamDescriptorsTable::DSMCCStreamDescriptorsTable(DuckContext& duck, const BinaryTable& table) :
    AbstractDescriptorsTable(duck, MY_TID, MY_XML_NAME, MY_STD, table),
    table_id_extension(_tid_ext)
{
}

ts::DSMCCStreamDescriptorsTable::DSMCCStreamDescriptorsTable(const ts::DSMCCStreamDescriptorsTable& other) :
    AbstractDescriptorsTable(other),
    table_id_extension(_tid_ext)
{
}

ts::DSMCCStreamDescriptorsTable& ts::DSMCCStreamDescriptorsTable::operator=(const DSMCCStreamDescriptorsTable& other)
{
    if (&other != this) {
        // Assign super class but leave uint16_t& table_id_extension unchanged.
        AbstractDescriptorsTable::operator=(other);
    }
    return *this;
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::DSMCCStreamDescriptorsTable::isPrivate() const
{
    return false; // MPEG-defined
}


//----------------------------------------------------------------------------
// A static method to display a section.
//----------------------------------------------------------------------------

void ts::DSMCCStreamDescriptorsTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Table id extension: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;
    AbstractDescriptorsTable::DisplaySection(disp, section, buf, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCStreamDescriptorsTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    AbstractDescriptorsTable::buildXML(duck, root);
    root->setIntAttribute(u"table_id_extension", _tid_ext, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCStreamDescriptorsTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return AbstractDescriptorsTable::analyzeXML(duck, element) &&
           element->getIntAttribute(_tid_ext, u"table_id_extension", false, 0xFFFF);
}

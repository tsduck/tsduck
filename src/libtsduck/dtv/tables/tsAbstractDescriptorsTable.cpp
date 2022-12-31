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

#include "tsAbstractDescriptorsTable.h"
#include "tsBinaryTable.h"
#include "tsPSIBuffer.h"
#include "tsSection.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractDescriptorsTable::AbstractDescriptorsTable(TID tid_, const UChar* xml_name, Standards standards, uint16_t tid_ext_, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid_, xml_name, standards, version_, is_current_),
    descs(this),
    _tid_ext(tid_ext_)
{
}

ts::AbstractDescriptorsTable::AbstractDescriptorsTable(const ts::AbstractDescriptorsTable& other) :
    AbstractLongTable(other),
    descs(this, other.descs),
    _tid_ext(other._tid_ext)
{

}

ts::AbstractDescriptorsTable::AbstractDescriptorsTable(DuckContext& duck, TID tid, const UChar* xml_name, Standards standards, const BinaryTable& table) :
    AbstractLongTable(tid, xml_name, standards, 0, true),
    descs(this),
    _tid_ext(0xFFFF)
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::AbstractDescriptorsTable::tableIdExtension() const
{
    return _tid_ext;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::clearContent()
{
    descs.clear();
    _tid_ext = 0xFFFF;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    _tid_ext = section.tableIdExtension();
    buf.getDescriptorList(descs);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    size_t start = 0;
    while (!buf.error() && start < descs.size()) {
        start = buf.putPartialDescriptorList(descs, start);
        addOneSection(table, buf);
    }
}


//----------------------------------------------------------------------------
// A static method to display a section.
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp.displayDescriptorList(section, buf, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractDescriptorsTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(is_current, u"current", false, true) &&
           descs.fromXML(duck, element);
}

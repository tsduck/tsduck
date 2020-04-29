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

#include "tsBIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"BIT"
#define MY_TID ts::TID_BIT
#define MY_STD ts::STD_ISDB

TS_XML_TABLE_FACTORY(ts::BIT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::BIT, MY_TID, MY_STD);
TS_FACTORY_REGISTER(ts::BIT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BIT::BIT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    broadcast_view_propriety(false),
    descs(this),
    broadcasters(this)
{
    _is_valid = true;
}

ts::BIT::BIT(const BIT& other) :
    AbstractLongTable(other),
    broadcast_view_propriety(other.broadcast_view_propriety),
    descs(this, other.descs),
    broadcasters(this, other.broadcasters)
{
}

ts::BIT::BIT(DuckContext& duck, const BinaryTable& table) :
    BIT()
{
    deserialize(duck, table);
}

ts::BIT::Broadcaster::Broadcaster(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::BIT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::BIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// A static method to display a BIT section.
//----------------------------------------------------------------------------

void ts::BIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::BIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    //@@@@@@@@@@@@
}

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    <BIT version="uint5, default=0"
         current="bool, default=true"
         original_network_id="uint16, required"
         broadcast_view_propriety="bool, required">
      <!-- Common descriptors loop -->
      <_any in="_descriptors"/>
      <!-- One per broadcaster -->
      <broadcaster broadcaster_id="uint8, required">
        <_any in="_descriptors"/>
      </broadcaster>
    </BIT>
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::BIT::fromXML(DuckContext& duck, const xml::Element* element)
{
    //@@@@@@@@@@@@
}

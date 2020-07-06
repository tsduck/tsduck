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

#include "tsCellFrequencyLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"cell_frequency_link_descriptor"
#define MY_CLASS ts::CellFrequencyLinkDescriptor
#define MY_DID ts::DID_CELL_FREQ_LINK
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CellFrequencyLinkDescriptor::CellFrequencyLinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cells()
{
}

ts::CellFrequencyLinkDescriptor::CellFrequencyLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    CellFrequencyLinkDescriptor()
{
    deserialize(duck, desc);
}

void ts::CellFrequencyLinkDescriptor::clearContent()
{
    cells.clear();
}

ts::CellFrequencyLinkDescriptor::Cell::Cell() :
    cell_id(0),
    frequency(0),
    subcells()
{
}

ts::CellFrequencyLinkDescriptor::Subcell::Subcell() :
    cell_id_extension(0),
    transposer_frequency(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        bbp->appendUInt16(it1->cell_id);
        bbp->appendUInt32(uint32_t(it1->frequency / 10)); // coded in 10 Hz unit
        bbp->appendUInt8(uint8_t(it1->subcells.size() * 5));
        for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
            bbp->appendUInt8(it2->cell_id_extension);
            bbp->appendUInt32(uint32_t(it2->transposer_frequency / 10)); // coded in 10 Hz unit
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag();
    cells.clear();

    while (_is_valid && size >= 7) {
        Cell cell;
        cell.cell_id = GetUInt16(data);
        cell.frequency = uint64_t(GetUInt32(data + 2)) * 10; // coded in 10 Hz unit
        size_t len = data[6];
        data += 7; size -= 7;

        while (size >= len && len >= 5) {
            Subcell sub;
            sub.cell_id_extension = data[0];
            sub.transposer_frequency = uint64_t(GetUInt32(data + 1)) * 10; // coded in 10 Hz unit
            cell.subcells.push_back(sub);
            data += 5; size -= 5; len -= 5;
        }

        _is_valid = len == 0;
        cells.push_back(cell);
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 7) {
        size_t len = data[6];
        strm << margin << UString::Format(u"- Cell id: 0x%X, frequency: %'d Hz", {GetUInt16(data), uint64_t(GetUInt32(data + 2)) * 10}) << std::endl;
        data += 7; size -= 7;

        while (size >= len && len >= 5) {
            strm << margin << UString::Format(u"  Subcell id ext: 0x%X, frequency: %'d Hz", {data[0], uint64_t(GetUInt32(data + 1)) * 10}) << std::endl;
            data += 5; size -= 5; len -= 5;
        }
        if (len > 0) {
            break;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1->cell_id, true);
        e1->setIntAttribute(u"frequency", it1->frequency);
        for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"subcell");
            e2->setIntAttribute(u"cell_id_extension", it2->cell_id_extension, true);
            e2->setIntAttribute(u"transposer_frequency", it2->transposer_frequency);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CellFrequencyLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcells;
    bool ok = element->getChildren(xcells, u"cell");

    for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
        Cell cell;
        xml::ElementVector xsubcells;
        ok = xcells[i1]->getIntAttribute<uint16_t>(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getIntAttribute<uint64_t>(cell.frequency, u"frequency", true) &&
             xcells[i1]->getChildren(xsubcells, u"subcell");
        for (size_t i2 = 0; ok && i2 < xsubcells.size(); ++i2) {
            Subcell sub;
            ok = xsubcells[i2]->getIntAttribute<uint8_t>(sub.cell_id_extension, u"cell_id_extension", true) &&
                 xsubcells[i2]->getIntAttribute<uint64_t>(sub.transposer_frequency, u"transposer_frequency", true);
            cell.subcells.push_back(sub);
        }
        cells.push_back(cell);
    }
    return ok;
}

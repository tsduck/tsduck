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

#include "tsCellListDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"cell_list_descriptor"
#define MY_CLASS ts::CellListDescriptor
#define MY_DID ts::DID_CELL_LIST
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CellListDescriptor::CellListDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cells()
{
}

void ts::CellListDescriptor::clearContent()
{
    cells.clear();
}

ts::CellListDescriptor::CellListDescriptor(DuckContext& duck, const Descriptor& desc) :
    CellListDescriptor()
{
    deserialize(duck, desc);
}

ts::CellListDescriptor::Cell::Cell() :
    cell_id(0),
    cell_latitude(0),
    cell_longitude(0),
    cell_extent_of_latitude(0),
    cell_extent_of_longitude(0),
    subcells()
{
}

ts::CellListDescriptor::Subcell::Subcell() :
    cell_id_extension(0),
    subcell_latitude(0),
    subcell_longitude(0),
    subcell_extent_of_latitude(0),
    subcell_extent_of_longitude(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CellListDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        bbp->appendUInt16(it1->cell_id);
        bbp->appendInt16(it1->cell_latitude);
        bbp->appendInt16(it1->cell_longitude);
        bbp->appendUInt24((uint32_t(it1->cell_extent_of_latitude & 0x0FFF) << 12) | (it1->cell_extent_of_longitude & 0x0FFF));
        bbp->appendUInt8(uint8_t(it1->subcells.size() * 8));
        for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
            bbp->appendUInt8(it2->cell_id_extension);
            bbp->appendInt16(it2->subcell_latitude);
            bbp->appendInt16(it2->subcell_longitude);
            bbp->appendUInt24((uint32_t(it2->subcell_extent_of_latitude & 0x0FFF) << 12) | (it2->subcell_extent_of_longitude & 0x0FFF));
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CellListDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag();
    cells.clear();

    while (_is_valid && size >= 10) {
        Cell cell;
        cell.cell_id = GetUInt16(data);
        cell.cell_latitude = GetInt16(data + 2);
        cell.cell_longitude = GetInt16(data + 4);
        uint32_t ext = GetUInt24(data + 6);
        cell.cell_extent_of_latitude = uint16_t(ext >> 12) & 0x0FFF;
        cell.cell_extent_of_longitude = uint16_t(ext) & 0x0FFF;
        size_t len = data[9];
        data += 10; size -= 10;

        while (size >= len && len >= 8) {
            Subcell sub;
            sub.cell_id_extension = data[0];
            sub.subcell_latitude = GetInt16(data + 1);
            sub.subcell_longitude = GetInt16(data + 3);
            ext = GetUInt24(data + 5);
            sub.subcell_extent_of_latitude = uint16_t(ext >> 12) & 0x0FFF;
            sub.subcell_extent_of_longitude = uint16_t(ext) & 0x0FFF;
            cell.subcells.push_back(sub);
            data += 8; size -= 8; len -= 8;
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

void ts::CellListDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 10) {
        strm << margin << UString::Format(u"- Cell id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
        DisplayCoordinates(display, data + 2, size - 2, indent + 2);
        size_t len = data[9];
        data += 10; size -= 10;

        while (size >= len && len >= 8) {
            strm << margin << UString::Format(u"  - Subcell id ext: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            DisplayCoordinates(display, data + 1, size - 1, indent + 4);
            data += 8; size -= 8; len -= 8;
        }
        if (len > 0) {
            break;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Static method to display coordinates of a cell or subcell.
//----------------------------------------------------------------------------

void ts::CellListDescriptor::DisplayCoordinates(TablesDisplay& display, const uint8_t* data, size_t size, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    int32_t latitude = GetInt16(data);
    int32_t longitude = GetInt16(data + 2);
    uint32_t ext = GetUInt24(data + 4);
    uint16_t lat_ext = uint16_t(ext >> 12) & 0x0FFF;
    uint16_t long_ext = uint16_t(ext) & 0x0FFF;

    strm << margin << UString::Format(u"Raw latitude/longitude: %d/%d, extent: %d/%d", {latitude, longitude, lat_ext, long_ext}) << std::endl
         << margin << "Actual latitude range: " << ToDegrees(latitude, true) << " to " << ToDegrees(latitude + lat_ext, true) << std::endl
         << margin << "Actual longitude range: " << ToDegrees(longitude, false) << " to " << ToDegrees(longitude + long_ext, false) << std::endl;
}


//----------------------------------------------------------------------------
// Static method to convert a raw latitude or longitude into a readable string.
//----------------------------------------------------------------------------

ts::UString ts::CellListDescriptor::ToDegrees(int32_t value, bool is_latitude)
{
    // Convert value as a positive value, in 2^15 degrees.
    UChar orientation = CHAR_NULL;
    if (is_latitude) {
        if (value >= 0) {
            orientation = 'N';
            value = value * 90;
        }
        else {
            orientation = 'S';
            value = -value * 90;
        }
    }
    else {
        if (value >= 0) {
            orientation = 'E';
            value = value * 180;
        }
        else {
            orientation = 'W';
            value = -value * 180;
        }
    }

    // Compute degrees, minutes, seconds.
    int32_t deg = value / 0x8000;
    int32_t sec = ((value % 0x8000) * 3600) / 0x8000;
    return UString::Format(u"%d%c %d' %d\" %c", {deg, MASCULINE_ORDINAL_INDICATOR, sec / 60, sec % 60, orientation});
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CellListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1->cell_id, true);
        e1->setIntAttribute(u"cell_latitude", it1->cell_latitude);
        e1->setIntAttribute(u"cell_longitude", it1->cell_longitude);
        e1->setIntAttribute(u"cell_extent_of_latitude", it1->cell_extent_of_latitude & 0x0FFF);
        e1->setIntAttribute(u"cell_extent_of_longitude", it1->cell_extent_of_longitude & 0x0FFF);
        for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"subcell");
            e2->setIntAttribute(u"cell_id_extension", it2->cell_id_extension, true);
            e2->setIntAttribute(u"subcell_latitude", it2->subcell_latitude);
            e2->setIntAttribute(u"subcell_longitude", it2->subcell_longitude);
            e2->setIntAttribute(u"subcell_extent_of_latitude", it2->subcell_extent_of_latitude & 0x0FFF);
            e2->setIntAttribute(u"subcell_extent_of_longitude", it2->subcell_extent_of_longitude & 0x0FFF);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CellListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcells;
    bool ok = element->getChildren(xcells, u"cell");

    for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
        Cell cell;
        xml::ElementVector xsubcells;
        ok = xcells[i1]->getIntAttribute<uint16_t>(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getIntAttribute<int16_t>(cell.cell_latitude, u"cell_latitude", true) &&
             xcells[i1]->getIntAttribute<int16_t>(cell.cell_longitude, u"cell_longitude", true) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.cell_extent_of_latitude, u"cell_extent_of_latitude", true, 0, 0, 0x0FFF) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.cell_extent_of_longitude, u"cell_extent_of_longitude", true, 0, 0, 0x0FFF) &&
             xcells[i1]->getChildren(xsubcells, u"subcell");
        for (size_t i2 = 0; ok && i2 < xsubcells.size(); ++i2) {
            Subcell sub;
            ok = xsubcells[i2]->getIntAttribute<uint8_t>(sub.cell_id_extension, u"cell_id_extension", true) &&
                 xsubcells[i2]->getIntAttribute<int16_t>(sub.subcell_latitude, u"subcell_latitude", true) &&
                 xsubcells[i2]->getIntAttribute<int16_t>(sub.subcell_longitude, u"subcell_longitude", true) &&
                 xsubcells[i2]->getIntAttribute<uint16_t>(sub.subcell_extent_of_latitude, u"subcell_extent_of_latitude", true, 0, 0, 0x0FFF) &&
                 xsubcells[i2]->getIntAttribute<uint16_t>(sub.subcell_extent_of_longitude, u"subcell_extent_of_longitude", true, 0, 0, 0x0FFF);
            cell.subcells.push_back(sub);
        }
        cells.push_back(cell);
    }
    return ok;
}

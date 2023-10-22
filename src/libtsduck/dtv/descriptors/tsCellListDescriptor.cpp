//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCellListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"cell_list_descriptor"
#define MY_CLASS ts::CellListDescriptor
#define MY_DID ts::DID_CELL_LIST
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CellListDescriptor::CellListDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CellListDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : cells) {
        buf.putUInt16(it1.cell_id);
        buf.putInt16(it1.cell_latitude);
        buf.putInt16(it1.cell_longitude);
        buf.putBits(it1.cell_extent_of_latitude, 12);
        buf.putBits(it1.cell_extent_of_longitude, 12);
        buf.pushWriteSequenceWithLeadingLength(8); // start write sequence
        for (const auto& it2 : it1.subcells) {
            buf.putUInt8(it2.cell_id_extension);
            buf.putInt16(it2.subcell_latitude);
            buf.putInt16(it2.subcell_longitude);
            buf.putBits(it2.subcell_extent_of_latitude, 12);
            buf.putBits(it2.subcell_extent_of_longitude, 12);
        }
        buf.popState(); // end write sequence
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CellListDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Cell cell;
        cell.cell_id = buf.getUInt16();
        cell.cell_latitude = buf.getInt16();
        cell.cell_longitude = buf.getInt16();
        buf.getBits(cell.cell_extent_of_latitude, 12);
        buf.getBits(cell.cell_extent_of_longitude, 12);
        buf.pushReadSizeFromLength(8); // start read sequence
        while (buf.canRead()) {
            Subcell sub;
            sub.cell_id_extension = buf.getUInt8();
            sub.subcell_latitude = buf.getInt16();
            sub.subcell_longitude = buf.getInt16();
            buf.getBits(sub.subcell_extent_of_latitude, 12);
            buf.getBits(sub.subcell_extent_of_longitude, 12);
            cell.subcells.push_back(sub);
        }
        buf.popState(); // end read sequence
        cells.push_back(cell);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CellListDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(10)) {
        disp << margin << UString::Format(u"- Cell id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        DisplayCoordinates(disp, buf, margin + u"  ");
        buf.pushReadSizeFromLength(8); // start read sequence
        while (buf.canReadBytes(8)) {
            disp << margin << UString::Format(u"  - Subcell id ext: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            DisplayCoordinates(disp, buf, margin + u"    ");
        }
        disp.displayPrivateData(u"Extraneous subcell data", buf, NPOS, margin + u"  ");
        buf.popState(); // end read sequence
    }
}


//----------------------------------------------------------------------------
// Static method to display coordinates of a cell or subcell.
//----------------------------------------------------------------------------

void ts::CellListDescriptor::DisplayCoordinates(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    const int32_t latitude = buf.getInt16();
    const int32_t longitude = buf.getInt16();
    const uint16_t lat_ext = buf.getBits<uint16_t>(12);
    const uint16_t long_ext = buf.getBits<uint16_t>(12);

    disp << margin << UString::Format(u"Raw latitude/longitude: %d/%d, extent: %d/%d", {latitude, longitude, lat_ext, long_ext}) << std::endl;
    disp << margin << "Actual latitude range: " << ToDegrees(latitude, true) << " to " << ToDegrees(latitude + lat_ext, true) << std::endl;
    disp << margin << "Actual longitude range: " << ToDegrees(longitude, false) << " to " << ToDegrees(longitude + long_ext, false) << std::endl;
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
    for (const auto& it1 : cells) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1.cell_id, true);
        e1->setIntAttribute(u"cell_latitude", it1.cell_latitude);
        e1->setIntAttribute(u"cell_longitude", it1.cell_longitude);
        e1->setIntAttribute(u"cell_extent_of_latitude", it1.cell_extent_of_latitude & 0x0FFF);
        e1->setIntAttribute(u"cell_extent_of_longitude", it1.cell_extent_of_longitude & 0x0FFF);
        for (const auto& it2 : it1.subcells) {
            xml::Element* e2 = e1->addElement(u"subcell");
            e2->setIntAttribute(u"cell_id_extension", it2.cell_id_extension, true);
            e2->setIntAttribute(u"subcell_latitude", it2.subcell_latitude);
            e2->setIntAttribute(u"subcell_longitude", it2.subcell_longitude);
            e2->setIntAttribute(u"subcell_extent_of_latitude", it2.subcell_extent_of_latitude & 0x0FFF);
            e2->setIntAttribute(u"subcell_extent_of_longitude", it2.subcell_extent_of_longitude & 0x0FFF);
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
        ok = xcells[i1]->getIntAttribute(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getIntAttribute(cell.cell_latitude, u"cell_latitude", true) &&
             xcells[i1]->getIntAttribute(cell.cell_longitude, u"cell_longitude", true) &&
             xcells[i1]->getIntAttribute(cell.cell_extent_of_latitude, u"cell_extent_of_latitude", true, 0, 0, 0x0FFF) &&
             xcells[i1]->getIntAttribute(cell.cell_extent_of_longitude, u"cell_extent_of_longitude", true, 0, 0, 0x0FFF) &&
             xcells[i1]->getChildren(xsubcells, u"subcell");
        for (size_t i2 = 0; ok && i2 < xsubcells.size(); ++i2) {
            Subcell sub;
            ok = xsubcells[i2]->getIntAttribute(sub.cell_id_extension, u"cell_id_extension", true) &&
                 xsubcells[i2]->getIntAttribute(sub.subcell_latitude, u"subcell_latitude", true) &&
                 xsubcells[i2]->getIntAttribute(sub.subcell_longitude, u"subcell_longitude", true) &&
                 xsubcells[i2]->getIntAttribute(sub.subcell_extent_of_latitude, u"subcell_extent_of_latitude", true, 0, 0, 0x0FFF) &&
                 xsubcells[i2]->getIntAttribute(sub.subcell_extent_of_longitude, u"subcell_extent_of_longitude", true, 0, 0, 0x0FFF);
            cell.subcells.push_back(sub);
        }
        cells.push_back(cell);
    }
    return ok;
}

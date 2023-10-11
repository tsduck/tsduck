//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMosaicDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"mosaic_descriptor"
#define MY_CLASS ts::MosaicDescriptor
#define MY_DID ts::DID_MOSAIC
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MosaicDescriptor::MosaicDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MosaicDescriptor::clearContent()
{
    mosaic_entry_point = false;
    number_of_horizontal_elementary_cells = 0;
    number_of_vertical_elementary_cells = 0;
    cells.clear();
}

ts::MosaicDescriptor::MosaicDescriptor(DuckContext& duck, const Descriptor& desc) :
    MosaicDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(mosaic_entry_point);
    buf.putBits(number_of_horizontal_elementary_cells, 3);
    buf.putBit(1);
    buf.putBits(number_of_vertical_elementary_cells, 3);

    for (const auto& it : cells) {
        buf.putBits(it.logical_cell_id, 6);
        buf.putBits(0xFF, 7);
        buf.putBits(it.logical_cell_presentation_info, 3);
        buf.pushWriteSequenceWithLeadingLength(8); // elementary_cell_field_length
        for (size_t i = 0; i < it.elementary_cell_ids.size(); ++i) {
            buf.putBits(0xFF, 2);
            buf.putBits(it.elementary_cell_ids[i], 6);
        }
        buf.popState(); // update elementary_cell_field_length
        buf.putUInt8(it.cell_linkage_info);

        switch (it.cell_linkage_info) {
            case 0x01:
                buf.putUInt16(it.bouquet_id);
                break;
            case 0x02:
            case 0x03:
                buf.putUInt16(it.original_network_id);
                buf.putUInt16(it.transport_stream_id);
                buf.putUInt16(it.service_id);
                break;
            case 0x04:
                buf.putUInt16(it.original_network_id);
                buf.putUInt16(it.transport_stream_id);
                buf.putUInt16(it.service_id);
                buf.putUInt16(it.event_id);
                break;
            default:
                break;
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::deserializePayload(PSIBuffer& buf)
{
    mosaic_entry_point = buf.getBool();
    buf.getBits(number_of_horizontal_elementary_cells, 3);
    buf.skipBits(1);
    buf.getBits(number_of_vertical_elementary_cells, 3);

    while (buf.canRead()) {
        Cell cell;
        buf.getBits(cell.logical_cell_id, 6);
        buf.skipBits(7);
        buf.getBits(cell.logical_cell_presentation_info, 3);
        buf.pushReadSizeFromLength(8); // elementary_cell_field_length
        while (buf.canRead()) {
            buf.skipBits(2);
            cell.elementary_cell_ids.push_back(buf.getBits<uint8_t>(6));
        }
        buf.popState(); // end of elementary_cell_field_length
        cell.cell_linkage_info = buf.getUInt8();

        switch (cell.cell_linkage_info) {
            case 0x01:
                cell.bouquet_id = buf.getUInt16();
                break;
            case 0x02:
            case 0x03:
                cell.original_network_id = buf.getUInt16();
                cell.transport_stream_id = buf.getUInt16();
                cell.service_id = buf.getUInt16();
                break;
            case 0x04:
                cell.original_network_id = buf.getUInt16();
                cell.transport_stream_id = buf.getUInt16();
                cell.service_id = buf.getUInt16();
                cell.event_id = buf.getUInt16();
                break;
            default:
                break;
        }
        cells.push_back(cell);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Mosaic entry point: %s", {buf.getBool()}) << std::endl;
        const uint8_t hor = buf.getBits<uint8_t>(3);
        disp << margin << UString::Format(u"Horizontal elementary cells: %d (actual number: %d)", {hor, hor + 1}) << std::endl;
        buf.skipBits(1);
        const uint8_t ver = buf.getBits<uint8_t>(3);
        disp << margin << UString::Format(u"Vertical elementary cells: %d (actual number: %d)", {ver, ver + 1}) << std::endl;
    }

    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"- Logical cell id: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
        buf.skipBits(7);
        disp << margin << "  Presentation info: " << DataName(MY_XML_NAME, u"LogicalCellPresentation", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;

        buf.pushReadSizeFromLength(8); // elementary_cell_field_length
        while (buf.canReadBytes(1)) {
            buf.skipBits(2);
            disp << margin << UString::Format(u"  Elementary cell id: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
        }
        buf.popState(); // end of elementary_cell_field_length

        const uint8_t link = buf.getUInt8();
        disp << margin << "  Cell linkage info: " << DataName(MY_XML_NAME, u"CellLinkageInfo", link, NamesFlags::DECIMAL_FIRST) << std::endl;

        if (link == 0x01 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"  Bouquet id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        else if ((link == 0x02 || link == 0x03) && buf.canReadBytes(6)) {
            disp << margin << UString::Format(u"  Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        else if (link == 0x04 && buf.canReadBytes(8)) {
            disp << margin << UString::Format(u"  Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Event id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"mosaic_entry_point", mosaic_entry_point);
    root->setIntAttribute(u"number_of_horizontal_elementary_cells", number_of_horizontal_elementary_cells);
    root->setIntAttribute(u"number_of_vertical_elementary_cells", number_of_vertical_elementary_cells);

    for (const auto& it : cells) {
        xml::Element* e = root->addElement(u"cell");
        e->setIntAttribute(u"logical_cell_id", it.logical_cell_id, true);
        e->setIntAttribute(u"logical_cell_presentation_info", it.logical_cell_presentation_info, true);
        e->setIntAttribute(u"cell_linkage_info", it.cell_linkage_info, true);
        for (size_t i = 0; i < it.elementary_cell_ids.size(); ++i) {
            e->addElement(u"elementary_cell")->setIntAttribute(u"id", it.elementary_cell_ids[i], true);
        }
        switch (it.cell_linkage_info) {
            case 0x01:
                e->setIntAttribute(u"bouquet_id", it.bouquet_id, true);
                break;
            case 0x02:
            case 0x03:
                e->setIntAttribute(u"original_network_id", it.original_network_id, true);
                e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
                e->setIntAttribute(u"service_id", it.service_id, true);
                break;
            case 0x04:
                e->setIntAttribute(u"original_network_id", it.original_network_id, true);
                e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
                e->setIntAttribute(u"service_id", it.service_id, true);
                e->setIntAttribute(u"event_id", it.event_id, true);
                break;
            default:
                break;
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MosaicDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcells;
    bool ok =
        element->getBoolAttribute(mosaic_entry_point, u"mosaic_entry_point", true) &&
        element->getIntAttribute(number_of_horizontal_elementary_cells, u"number_of_horizontal_elementary_cells", true, 0, 0, 7) &&
        element->getIntAttribute(number_of_vertical_elementary_cells, u"number_of_vertical_elementary_cells", true, 0, 0, 7) &&
        element->getChildren(xcells, u"cell");

    for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
        Cell cell;
        xml::ElementVector xids;
        ok = xcells[i1]->getIntAttribute(cell.logical_cell_id, u"logical_cell_id", true, 0, 0x00, 0x3F) &&
             xcells[i1]->getIntAttribute(cell.logical_cell_presentation_info, u"logical_cell_presentation_info", true, 0, 0x00, 0x07) &&
             xcells[i1]->getIntAttribute(cell.cell_linkage_info, u"cell_linkage_info", true) &&
             xcells[i1]->getIntAttribute(cell.bouquet_id, u"bouquet_id", cell.cell_linkage_info == 1) &&
             xcells[i1]->getIntAttribute(cell.original_network_id, u"original_network_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute(cell.transport_stream_id, u"transport_stream_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute(cell.service_id, u"service_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute(cell.event_id, u"event_id", cell.cell_linkage_info == 4) &&
             xcells[i1]->getChildren(xids, u"elementary_cell");
        for (size_t i2 = 0; ok && i2 < xids.size(); ++i2) {
            uint8_t id = 0;
            ok = xids[i2]->getIntAttribute(id, u"id", true, 0, 0x00, 0x3F);
            cell.elementary_cell_ids.push_back(id);
        }
        cells.push_back(cell);
    }
    return ok;
}

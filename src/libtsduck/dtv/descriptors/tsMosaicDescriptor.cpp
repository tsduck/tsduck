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

#include "tsMosaicDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"mosaic_descriptor"
#define MY_CLASS ts::MosaicDescriptor
#define MY_DID ts::DID_MOSAIC
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MosaicDescriptor::MosaicDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    mosaic_entry_point(false),
    number_of_horizontal_elementary_cells(0),
    number_of_vertical_elementary_cells(0),
    cells()
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

ts::MosaicDescriptor::Cell::Cell() :
    logical_cell_id(0),
    logical_cell_presentation_info(0),
    elementary_cell_ids(),
    cell_linkage_info(0),
    bouquet_id(0),
    original_network_id(0),
    transport_stream_id(0),
    service_id(0),
    event_id(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((mosaic_entry_point ? 0x88 : 0x08) |
                     uint8_t((number_of_horizontal_elementary_cells & 0x07) << 4) |
                     (number_of_vertical_elementary_cells & 0x07));
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        bbp->appendUInt8(uint8_t(it->logical_cell_id << 2) | 0x03);
        bbp->appendUInt8(0xF8 | (it->logical_cell_presentation_info & 0x07));
        bbp->appendUInt8(uint8_t(it->elementary_cell_ids.size()));
        for (size_t i = 0; i < it->elementary_cell_ids.size(); ++i) {
            bbp->appendUInt8(0xC0 | it->elementary_cell_ids[i]);
        }
        bbp->appendUInt8(it->cell_linkage_info);
        switch (it->cell_linkage_info) {
            case 0x01:
                bbp->appendUInt16(it->bouquet_id);
                break;
            case 0x02:
            case 0x03:
                bbp->appendUInt16(it->original_network_id);
                bbp->appendUInt16(it->transport_stream_id);
                bbp->appendUInt16(it->service_id);
                break;
            case 0x04:
                bbp->appendUInt16(it->original_network_id);
                bbp->appendUInt16(it->transport_stream_id);
                bbp->appendUInt16(it->service_id);
                bbp->appendUInt16(it->event_id);
                break;
            default:
                break;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;
    cells.clear();

    if (_is_valid) {
        mosaic_entry_point = (data[0] & 0x80) != 0;
        number_of_horizontal_elementary_cells = (data[0] >> 4) & 0x07;
        number_of_vertical_elementary_cells = data[0] & 0x07;
        data++; size--;
    }

    while (_is_valid && size >= 3) {
        Cell cell;
        cell.logical_cell_id = (data[0] >> 2) & 0x3F;
        cell.logical_cell_presentation_info = data[1] & 0x07;
        size_t len = data[2];
        data += 3; size -= 3;

        _is_valid = size > len;
        if (_is_valid) {
            for (size_t i = 0; i < len; ++i) {
                cell.elementary_cell_ids.push_back(data[i] & 0x3F);
            }
            cell.cell_linkage_info = data[len];
            data += len + 1; size -= len + 1;

            switch (cell.cell_linkage_info) {
                case 0x01:
                    _is_valid = size >= 2;
                    if (_is_valid) {
                        cell.bouquet_id = GetUInt16(data);
                        data += 2; size -= 2;
                    }
                    break;
                case 0x02:
                case 0x03:
                    _is_valid = size >= 6;
                    if (_is_valid) {
                        cell.original_network_id = GetUInt16(data);
                        cell.transport_stream_id = GetUInt16(data + 2);
                        cell.service_id = GetUInt16(data + 4);
                        data += 6; size -= 6;
                    }
                    break;
                case 0x04:
                    _is_valid = size >= 8;
                    if (_is_valid) {
                        cell.original_network_id = GetUInt16(data);
                        cell.transport_stream_id = GetUInt16(data + 2);
                        cell.service_id = GetUInt16(data + 4);
                        cell.event_id = GetUInt16(data + 6);
                        data += 8; size -= 8;
                    }
                    break;
                default:
                    break;
            }
        }

        if (_is_valid) {
            cells.push_back(cell);
        }
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = size >= 1;

    if (ok) {
        const uint8_t hor = (data[0] >> 4) & 0x07;
        const uint8_t ver = data[0] & 0x07;
        strm << margin << UString::Format(u"Mosaic entry point: %s", {(data[0] & 0x80) != 0}) << std::endl
             << margin << UString::Format(u"Horizontal elementary cells: %d (actual number: %d)", {hor, hor + 1}) << std::endl
             << margin << UString::Format(u"Vertical elementary cells: %d (actual number: %d)", {ver, ver + 1}) << std::endl;
        data++; size--;
    }

    while (ok && size >= 3) {
        const uint8_t id = (data[0] >> 2) & 0x3F;
        const uint8_t pres = data[1] & 0x07;
        size_t len = data[2];
        data += 3; size -= 3;

        strm << margin << UString::Format(u"- Logical cell id: 0x%X (%d)", {id, id}) << std::endl
             << margin << "  Presentation info: " << NameFromSection(u"MosaicLogicalCellPresentation", pres, names::DECIMAL_FIRST) << std::endl;

        ok = size > len;
        if (ok) {
            for (size_t i = 0; i < len; ++i) {
                const uint8_t eid = data[i] & 0x3F;
                strm << margin << UString::Format(u"  Elementary cell id: 0x%X (%d)", {eid, eid}) << std::endl;
            }
            const uint8_t link = data[len];
            strm << margin << "  Cell linkage info: " << NameFromSection(u"MosaicCellLinkageInfo", link, names::DECIMAL_FIRST) << std::endl;
            data += len + 1; size -= len + 1;

            switch (link) {
                case 0x01:
                    ok = size >= 2;
                    if (ok) {
                        strm << margin << UString::Format(u"  Bouquet id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
                        data += 2; size -= 2;
                    }
                    break;
                case 0x02:
                case 0x03:
                    ok = size >= 6;
                    if (ok) {
                        strm << margin << UString::Format(u"  Original network id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                             << margin << UString::Format(u"  Transport stream id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
                             << margin << UString::Format(u"  Service id: 0x%X (%d)", {GetUInt16(data + 4), GetUInt16(data + 4)}) << std::endl;
                        data += 6; size -= 6;
                    }
                    break;
                case 0x04:
                    ok = size >= 8;
                    if (ok) {
                        strm << margin << UString::Format(u"  Original network id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                             << margin << UString::Format(u"  Transport stream id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
                             << margin << UString::Format(u"  Service id: 0x%X (%d)", {GetUInt16(data + 4), GetUInt16(data + 4)}) << std::endl
                             << margin << UString::Format(u"  Event id: 0x%X (%d)", {GetUInt16(data + 6), GetUInt16(data + 6)}) << std::endl;
                        data += 8; size -= 8;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MosaicDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"mosaic_entry_point", mosaic_entry_point);
    root->setIntAttribute(u"number_of_horizontal_elementary_cells", number_of_horizontal_elementary_cells);
    root->setIntAttribute(u"number_of_vertical_elementary_cells", number_of_vertical_elementary_cells);

    for (auto it = cells.begin(); it != cells.end(); ++it) {
        xml::Element* e = root->addElement(u"cell");
        e->setIntAttribute(u"logical_cell_id", it->logical_cell_id, true);
        e->setIntAttribute(u"logical_cell_presentation_info", it->logical_cell_presentation_info, true);
        e->setIntAttribute(u"cell_linkage_info", it->cell_linkage_info, true);
        for (size_t i = 0; i < it->elementary_cell_ids.size(); ++i) {
            e->addElement(u"elementary_cell")->setIntAttribute(u"id", it->elementary_cell_ids[i], true);
        }
        switch (it->cell_linkage_info) {
            case 0x01:
                e->setIntAttribute(u"bouquet_id", it->bouquet_id, true);
                break;
            case 0x02:
            case 0x03:
                e->setIntAttribute(u"original_network_id", it->original_network_id, true);
                e->setIntAttribute(u"transport_stream_id", it->transport_stream_id, true);
                e->setIntAttribute(u"service_id", it->service_id, true);
                break;
            case 0x04:
                e->setIntAttribute(u"original_network_id", it->original_network_id, true);
                e->setIntAttribute(u"transport_stream_id", it->transport_stream_id, true);
                e->setIntAttribute(u"service_id", it->service_id, true);
                e->setIntAttribute(u"event_id", it->event_id, true);
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
        element->getIntAttribute<uint8_t>(number_of_horizontal_elementary_cells, u"number_of_horizontal_elementary_cells", true, 0, 0, 7) &&
        element->getIntAttribute<uint8_t>(number_of_vertical_elementary_cells, u"number_of_vertical_elementary_cells", true, 0, 0, 7) &&
        element->getChildren(xcells, u"cell");

    for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
        Cell cell;
        xml::ElementVector xids;
        ok = xcells[i1]->getIntAttribute<uint8_t>(cell.logical_cell_id, u"logical_cell_id", true, 0, 0x00, 0x3F) &&
             xcells[i1]->getIntAttribute<uint8_t>(cell.logical_cell_presentation_info, u"logical_cell_presentation_info", true, 0, 0x00, 0x07) &&
             xcells[i1]->getIntAttribute<uint8_t>(cell.cell_linkage_info, u"cell_linkage_info", true) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.bouquet_id, u"bouquet_id", cell.cell_linkage_info == 1) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.original_network_id, u"original_network_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.transport_stream_id, u"transport_stream_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.service_id, u"service_id", cell.cell_linkage_info >= 2 && cell.cell_linkage_info <= 4) &&
             xcells[i1]->getIntAttribute<uint16_t>(cell.event_id, u"event_id", cell.cell_linkage_info == 4) &&
             xcells[i1]->getChildren(xids, u"elementary_cell");
        for (size_t i2 = 0; ok && i2 < xids.size(); ++i2) {
            uint8_t id = 0;
            ok = xids[i2]->getIntAttribute<uint8_t>(id, u"id", true, 0, 0x00, 0x3F);
            cell.elementary_cell_ids.push_back(id);
        }
        cells.push_back(cell);
    }
    return ok;
}

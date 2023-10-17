//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCellFrequencyLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"cell_frequency_link_descriptor"
#define MY_CLASS ts::CellFrequencyLinkDescriptor
#define MY_DID ts::DID_CELL_FREQ_LINK
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CellFrequencyLinkDescriptor::CellFrequencyLinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : cells) {
        buf.putUInt16(it1.cell_id);
        buf.putUInt32(uint32_t(it1.frequency / 10)); // coded in 10 Hz unit
        buf.pushWriteSequenceWithLeadingLength(8);    // start write sequence
        for (const auto& it2 : it1.subcells) {
            buf.putUInt8(it2.cell_id_extension);
            buf.putUInt32(uint32_t(it2.transposer_frequency / 10)); // coded in 10 Hz unit
        }
        buf.popState(); // end write sequence
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Cell cell;
        cell.cell_id = buf.getUInt16();
        cell.frequency = uint64_t(buf.getUInt32()) * 10; // coded in 10 Hz unit
        buf.pushReadSizeFromLength(8); // start read sequence
        while (buf.canRead()) {
            Subcell sub;
            sub.cell_id_extension = buf.getUInt8();
            sub.transposer_frequency = uint64_t(buf.getUInt32()) * 10; // coded in 10 Hz unit
            cell.subcells.push_back(sub);
        }
        buf.popState(); // end read sequence
        cells.push_back(cell);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"- Cell id: 0x%X", {buf.getUInt16()});
        disp << UString::Format(u", frequency: %'d Hz", {10 * uint64_t(buf.getUInt32())}) << std::endl;
        buf.pushReadSizeFromLength(8); // start read sequence
        while (buf.canRead()) {
            disp << margin << UString::Format(u"  Subcell id ext: 0x%X", {buf.getUInt8()});
            disp << UString::Format(u", frequency: %'d Hz", {10 * uint64_t(buf.getUInt32())}) << std::endl;
        }
        buf.popState(); // end read sequence
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CellFrequencyLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : cells) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1.cell_id, true);
        e1->setIntAttribute(u"frequency", it1.frequency);
        for (const auto& it2 : it1.subcells) {
            xml::Element* e2 = e1->addElement(u"subcell");
            e2->setIntAttribute(u"cell_id_extension", it2.cell_id_extension, true);
            e2->setIntAttribute(u"transposer_frequency", it2.transposer_frequency);
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
        ok = xcells[i1]->getIntAttribute(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getIntAttribute(cell.frequency, u"frequency", true) &&
             xcells[i1]->getChildren(xsubcells, u"subcell");
        for (size_t i2 = 0; ok && i2 < xsubcells.size(); ++i2) {
            Subcell sub;
            ok = xsubcells[i2]->getIntAttribute(sub.cell_id_extension, u"cell_id_extension", true) &&
                 xsubcells[i2]->getIntAttribute(sub.transposer_frequency, u"transposer_frequency", true);
            cell.subcells.push_back(sub);
        }
        cells.push_back(cell);
    }
    return ok;
}

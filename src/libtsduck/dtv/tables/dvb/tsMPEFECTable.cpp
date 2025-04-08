//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEFECTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPE_FEC"
#define MY_CLASS ts::MPEFECTable
#define MY_TID ts::TID_MPE_FEC
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEFECTable::MPEFECTable() :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, VERSION, CURRENT)
{
}

ts::MPEFECTable::MPEFECTable(DuckContext& duck, const BinaryTable& table) :
    MPEFECTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

void ts::MPEFECTable::clearContent()
{
    padding_columns = 0;
    columns.clear();
}

uint8_t ts::MPEFECTable::version() const
{
    return VERSION;
}

void ts::MPEFECTable::setVersion(uint8_t version)
{
    _version = VERSION;
}

bool ts::MPEFECTable::isCurrent() const
{
    return CURRENT;
}

void ts::MPEFECTable::setCurrent(bool is_current)
{
    _is_current = CURRENT;
}

bool ts::MPEFECTable::isPrivate() const
{
    // According to ISO/IEC 13818-6, section 9.2.2, in all DSM-CC sections, "the private_indicator field
    // shall be set to the complement of the section_syntax_indicator value". For long sections, the
    // syntax indicator is always 1 and, therefore, the private indicator shall always be 0 ("non-private").
    return false;
}

uint16_t ts::MPEFECTable::tableIdExtension() const
{
    return uint16_t(uint16_t(padding_columns) << 8) | 0x00FF;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEFECTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Section #n contains the column #n.
    const size_t index = section.sectionNumber();
    if (columns.size() <= index) {
        columns.reserve(section.lastSectionNumber() + 1);
        columns.resize(index + 1);
    }

    padding_columns = uint8_t(section.tableIdExtension() >> 8);
    columns[index].rt.deserialize(buf);
    buf.getBytes(columns[index].rs_data);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEFECTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // There must be at least one column.
    if (columns.empty() || columns.size() > MAX_COLUMN_NUMBER + 1) {
        buf.setUserError();
        return;
    }

    for (size_t i = 0; i < columns.size(); ++i) {
        columns[i].rt.serializePayload(buf);
        buf.putBytes(columns[i].rs_data);
        if (i < columns.size() - 1) {
            addOneSection(table, buf);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a MPEFECTable section.
//----------------------------------------------------------------------------

void ts::MPEFECTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Padding columns: " << (section.tableIdExtension() >> 8) << std::endl;

    if (buf.canReadBytes(4)) {
        MPERealTimeParameters::Display(disp, buf, margin, false);
        disp.displayPrivateData(u"- RS data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEFECTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"padding_columns", padding_columns);
    for (const auto& col : columns) {
        xml::Element* e = root->addElement(u"column");
        col.rt.buildXML(duck, e, false);
        e->addHexaTextChild(u"rs_data", col.rs_data, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEFECTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcol;
    bool ok = element->getIntAttribute(padding_columns, u"padding_columns", true, 0, 0, MAX_COLUMN_NUMBER) &&
              element->getChildren(xcol, u"column", 1, MAX_COLUMN_NUMBER + 1);

    columns.resize(xcol.size());
    for (size_t i = 0; ok && i < xcol.size(); ++i) {
        ok = columns[i].rt.analyzeXML(duck, xcol[i], false) &&
             xcol[i]->getHexaTextChild(columns[i].rs_data, u"rs_data", true);
    }
    return ok;
}

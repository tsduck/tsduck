//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEIFECTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPE_IFEC"
#define MY_CLASS ts::MPEIFECTable
#define MY_TID ts::TID_MPE_IFEC
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEIFECTable::MPEIFECTable() :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, VERSION, CURRENT)
{
}

ts::MPEIFECTable::MPEIFECTable(DuckContext& duck, const BinaryTable& table) :
    MPEIFECTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

void ts::MPEIFECTable::clearContent()
{
    burst_number = 0;
    IFEC_burst_size = 0;
    bursts.clear();
}

uint8_t ts::MPEIFECTable::version() const
{
    return VERSION;
}

void ts::MPEIFECTable::setVersion(uint8_t version)
{
    _version = VERSION;
}

bool ts::MPEIFECTable::isCurrent() const
{
    return CURRENT;
}

void ts::MPEIFECTable::setCurrent(bool is_current)
{
    _is_current = CURRENT;
}

bool ts::MPEIFECTable::isPrivate() const
{
    // According to ISO/IEC 13818-6, section 9.2.2, in all DSM-CC sections, "the private_indicator field
    // shall be set to the complement of the section_syntax_indicator value". For long sections, the
    // syntax indicator is always 1 and, therefore, the private indicator shall always be 0 ("non-private").
    return false;
}

uint16_t ts::MPEIFECTable::tableIdExtension() const
{
    return uint16_t(uint16_t(burst_number) << 8) | IFEC_burst_size;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEIFECTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Section #n contains the column #n.
    const size_t index = section.sectionNumber();
    if (bursts.size() <= index) {
        bursts.reserve(section.lastSectionNumber() + 1);
        bursts.resize(index + 1);
    }

    burst_number = uint8_t(section.tableIdExtension() >> 8);
    IFEC_burst_size = uint8_t(section.tableIdExtension() & 0x00FF);
    bursts[index].rt.deserialize(buf);
    buf.getBytes(bursts[index].IFEC_data);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEIFECTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // There must be at least one column.
    if (bursts.empty() || bursts.size() > 256) {
        buf.setUserError();
        return;
    }

    for (size_t i = 0; i < bursts.size(); ++i) {
        bursts[i].rt.serializePayload(buf);
        buf.putBytes(bursts[i].IFEC_data);
        if (i < bursts.size() - 1) {
            addOneSection(table, buf);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a MPEIFECTable section.
//----------------------------------------------------------------------------

void ts::MPEIFECTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Burst number: " << (section.tableIdExtension() >> 8) << std::endl;
    disp << margin << "IFEC burst size: " << (section.tableIdExtension() & 0x00FF) << std::endl;

    if (buf.canReadBytes(4)) {
        MPERealTimeParameters::Display(disp, buf, margin, true);
        disp.displayPrivateData(u"- IFEC data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEIFECTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"burst_number", burst_number);
    root->setIntAttribute(u"IFEC_burst_size", IFEC_burst_size);
    for (const auto& br : bursts) {
        xml::Element* e = root->addElement(u"burst");
        br.rt.buildXML(duck, e, true);
        e->addHexaTextChild(u"IFEC_data", br.IFEC_data, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEIFECTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xbr;
    bool ok = element->getIntAttribute(burst_number, u"burst_number", true) &&
              element->getIntAttribute(IFEC_burst_size, u"IFEC_burst_size", true) &&
              element->getChildren(xbr, u"burst", 1, 256);

    bursts.resize(xbr.size());
    for (size_t i = 0; ok && i < xbr.size(); ++i) {
        ok = bursts[i].rt.analyzeXML(duck, xbr[i], true) &&
             xbr[i]->getHexaTextChild(bursts[i].IFEC_data, u"IFEC_data", true);
    }
    return ok;
}

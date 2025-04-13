//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAETT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AETT"
#define MY_CLASS ts::AETT
#define MY_TID ts::TID_AETT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AETT::AETT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true) // AETT is always "current"
{
}

ts::AETT::AETT(DuckContext& duck, const BinaryTable& table) :
    AETT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::AETT::tableIdExtension() const
{
    return uint16_t(uint16_t(AETT_subtype) << 8) | MGT_tag;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::AETT::clearContent()
{
    AETT_subtype = 0;
    MGT_tag = 0;
    etms.clear();
    reserved.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AETT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    AETT_subtype = uint8_t(section.tableIdExtension() >> 8);
    MGT_tag = uint8_t(section.tableIdExtension());
    if (AETT_subtype == 0) {
        size_t num_blocks_in_section = buf.getUInt8();
        while (num_blocks_in_section-- > 0) {
            ETM& etm(etms.emplace_back());
            etm.ETM_id = buf.getUInt32();
            buf.skipReservedBits(4);
            buf.getMultipleString(etm.extended_text_message, buf.getBits<size_t>(12));
        }
    }
    else {
        buf.getBytes(reserved);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AETT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    if (AETT_subtype == 0) {
        // Save position before num_blocks_in_section. Will be updated at each ETM.
        uint8_t num_blocks_in_section = 0;
        buf.pushState();
        buf.putUInt8(num_blocks_in_section);
        const size_t payload_min_size = buf.currentWriteByteOffset();

        // Loop on ETM definitions.
        for (const auto& etm : etms) {
            // Pre-serialize the text. Its max size is 4095 bytes since its size must fit in 12 bits.
            ByteBlock message;
            etm.extended_text_message.serialize(buf.duck(), message, 4095, false);

            // Binary size of the ETM definition.
            const size_t etm_size = 6 + message.size();

            // If we are not at the beginning of the ETM loop, make sure that the entire
            // ETM fits in the section. If it does not fit, start a new section.
            if (etm_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
                // Create a new section.
                addOneSection(table, buf);
                // We are at the position of num_blocks_in_section in the new section.
                buf.putUInt8(num_blocks_in_section = 0);
            }

            // Serialize the ETM definition.
            buf.putUInt32(etm.ETM_id);
            buf.putReserved(4);
            buf.putBits(message.size(), 12);
            buf.putBytes(message);

            // Now increment the field num_blocks_in_section at saved position.
            buf.swapState();
            buf.pushState();
            buf.putUInt8(++num_blocks_in_section);
            buf.popState();
            buf.swapState();
        }
    }
    else {
        // Assume only one section in that case.
        buf.putBytes(reserved);
    }
}


//----------------------------------------------------------------------------
// A static method to display a AETT section.
//----------------------------------------------------------------------------

void ts::AETT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    const uint8_t AETT_subtype = uint8_t(section.tableIdExtension() >> 8);
    disp << margin << UString::Format(u"AETT subtype: %n, MGT tag: %n", AETT_subtype, uint8_t(section.tableIdExtension())) << std::endl;
    if (AETT_subtype != 0) {
        disp.displayPrivateData(u"Reserved", buf, NPOS, margin);
    }
    else if (buf.canReadBytes(1)) {
        const size_t num_blocks_in_section = buf.getUInt8();
        disp << margin << "Number of ETM: " << num_blocks_in_section << std::endl;
        for (size_t i = 0; buf.canReadBytes(6) && i < num_blocks_in_section; ++i) {
            disp << margin << UString::Format(u"- ETM #%d: ETM id: %n", i, buf.getUInt32()) << std::endl;
            buf.skipReservedBits(4);
            buf.pushReadSizeFromLength(12);
            disp.displayATSCMultipleString(buf, 0, margin + u"  ", u"Extended text message: ");
            buf.popState();
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AETT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"AETT_subtype", AETT_subtype, true);
    root->setIntAttribute(u"MGT_tag", MGT_tag, true);
    if (AETT_subtype != 0) {
        root->addHexaTextChild(u"reserved", reserved, true);
    }
    else {
        for (const auto& etm : etms) {
            xml::Element* e = root->addElement(u"ETM_data");
            e->setIntAttribute(u"ETM_id", etm.ETM_id, true);
            etm.extended_text_message.toXML(duck, e, u"extended_text_message", true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AETT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xetm;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getIntAttribute(AETT_subtype, u"AETT_subtype", false, 0) &&
              element->getIntAttribute(MGT_tag, u"MGT_tag", true) &&
              element->getChildren(xetm, u"ETM_data", 0, AETT_subtype != 0 ? 0 : xml::UNLIMITED) &&
              (AETT_subtype == 0 || element->getHexaTextChild(reserved, u"reserved"));

    if (ok && AETT_subtype == 0) {
        for (const auto& xe : xetm) {
            ETM& etm(etms.emplace_back());
            ok = xe->getIntAttribute(etm.ETM_id, u"ETM_id", true) &&
                 etm.extended_text_message.fromXML(duck, xe, u"extended_text_message", false) &&
                 ok;
        }
    }

    return ok;
}

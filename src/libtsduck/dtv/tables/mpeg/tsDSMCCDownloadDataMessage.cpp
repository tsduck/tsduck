//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCDownloadDataMessage.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSMCC_download_data_message"
#define MY_CLASS    ts::DSMCCDownloadDataMessage
#define MY_TID      ts::TID_DSMCC_DDM
#define MY_STD      ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCDownloadDataMessage::DSMCCDownloadDataMessage(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur)
{
}

ts::DSMCCDownloadDataMessage::DSMCCDownloadDataMessage(DuckContext& duck, const BinaryTable& table) :
    DSMCCDownloadDataMessage(0, true)
{
    deserialize(duck, table);
}

//----------------------------------------------------------------------------
// DSM-CC Message Header
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::DownloadDataHeader::clear()
{
    protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;
    dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;
    message_id = 0;
    download_id = 0;
}

void ts::DSMCCDownloadDataMessage::clearContent()
{
    table_id_ext = 0;
    header.clear();
    module_id = 0;
    module_version = 0;
    block_data.clear();
}

//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::DSMCCDownloadDataMessage::isPrivate() const
{
    // According to ISO/IEC 13818-6, section 9.2.2, in all DSM-CC sections, "the private_indicator field
    // shall be set to the complement of the section_syntax_indicator value". For long sections, the
    // syntax indicator is always 1 and, therefore, the private indicator shall always be 0 ("non-private").
    return false;
}

size_t ts::DSMCCDownloadDataMessage::maxPayloadSize() const
{
    // Although declared as a "non-private section" in the MPEG sense, the
    // DSM-CC section can use up to 4096 bytes according to
    // ETSI TS 102 809 V1.3.1 (2017-06), Table B.2.
    //
    // The maximum section length is 4096 bytes for all types of sections used in object carousel.
    // The section overhead is 12 bytes, leaving a maxium 4084 of payload per section.
    return MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
}

uint16_t ts::DSMCCDownloadDataMessage::tableIdExtension() const
{
    return module_id;
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::deserializePayload(PSIBuffer& buf, const Section& section)
{
    table_id_ext = section.tableIdExtension();
    header.protocol_discriminator = buf.getUInt8();
    header.dsmcc_type = buf.getUInt8();
    header.message_id = buf.getUInt16();
    header.download_id = buf.getUInt32();

    buf.skipBytes(1);  // reserved

    uint8_t adaptation_length = buf.getUInt8();

    buf.skipBytes(2);  // message_length

    // For object carousel it should be 0
    if (adaptation_length > 0) {
        buf.skipBytes(adaptation_length);
    }

    module_id = buf.getUInt16();
    module_version = buf.getUInt8();

    buf.skipBytes(1);  // reserved
    buf.skipBytes(2);  // block_number

    buf.getBytesAppend(block_data);
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------
void ts::DSMCCDownloadDataMessage::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putUInt8(header.protocol_discriminator);
    buf.putUInt8(header.dsmcc_type);
    buf.putUInt16(header.message_id);
    buf.putUInt32(header.download_id);

    buf.putUInt8(0xFF);  // reserved
    buf.putUInt8(0x00);  // adaptation_length

    buf.pushState();

    uint16_t block_number = 0x0000;
    size_t   block_data_index = 0;

    while (block_data_index < block_data.size()) {

        buf.pushWriteSequenceWithLeadingLength(16);  // message_length
        buf.putUInt16(module_id);
        buf.putUInt8(module_version);
        buf.putUInt8(0xFF);  // reserved

        buf.putUInt16(block_number);
        block_data_index += buf.putBytes(block_data, block_data_index, std::min(block_data.size() - block_data_index, buf.remainingWriteBytes()));

        buf.popState();  // message_length

        addOneSection(table, buf);

        block_number++;
    }
}

void ts::DSMCCDownloadDataMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    const uint16_t tidext = section.tableIdExtension();

    disp << margin << UString::Format(u"Table extension id: %n", tidext) << std::endl;

    if (buf.canReadBytes(DOWNLOAD_DATA_HEADER_SIZE)) {
        const uint8_t  protocol_discriminator = buf.getUInt8();
        const uint8_t  dsmcc_type = buf.getUInt8();
        const uint16_t message_id = buf.getUInt16();
        const uint32_t download_id = buf.getUInt32();

        buf.skipBytes(1);  // reserved

        uint16_t adaptation_length = buf.getUInt8();

        buf.skipBytes(2);  // message_length

        // For object carousel it should be 0
        if (adaptation_length > 0) {
            buf.skipBytes(adaptation_length);
        }

        disp << margin << UString::Format(u"Protocol discriminator: %n", protocol_discriminator) << std::endl;
        disp << margin << "Dsmcc type: " << DataName(MY_XML_NAME, u"dsmcc_type", dsmcc_type, NamesFlags::HEX_VALUE_NAME) << std::endl;
        if (dsmcc_type == DSMCC_TYPE_DOWNLOAD_MESSAGE) {
            disp << margin << "Message id: " << DataName(MY_XML_NAME, u"message_id", message_id, NamesFlags::HEX_VALUE_NAME) << std::endl;
        }
        else {
            disp << margin << UString::Format(u"Message id: %n", message_id) << std::endl;
        }
        disp << margin << UString::Format(u"Download id: %n", download_id) << std::endl;
    }

    if (buf.canReadBytes(6)) {
        const uint16_t module_id = buf.getUInt16();
        const uint8_t  module_version = buf.getUInt8();

        buf.skipBytes(1);

        const uint16_t block_number = buf.getUInt16();

        disp << margin << UString::Format(u"Module id: %n", module_id) << std::endl;
        disp << margin << UString::Format(u"Module version: %n", module_version) << std::endl;
        disp << margin << UString::Format(u"Block number: %n", block_number) << std::endl;

        disp.displayPrivateData(u"Block data:", buf, NPOS, margin);
    }
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"table_id_extension", table_id_ext, true);
    root->setIntAttribute(u"protocol_discriminator", header.protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", header.dsmcc_type, true);
    root->setIntAttribute(u"message_id", header.message_id, true);
    root->setIntAttribute(u"download_id", header.download_id, true);
    root->setIntAttribute(u"module_id", module_id, true);
    root->setIntAttribute(u"module_version", module_version, true);
    root->addHexaTextChild(u"block_data", block_data, true);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCDownloadDataMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(_is_current, u"current", false, true) &&
           element->getIntAttribute(table_id_ext, u"table_id_extension", true) &&
           element->getIntAttribute(header.protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
           element->getIntAttribute(header.dsmcc_type, u"dsmcc_type", true, 0x03) &&
           element->getIntAttribute(header.message_id, u"message_id", true) &&
           element->getIntAttribute(header.download_id, u"download_id", true) &&
           element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_version, u"module_version", true) &&
           element->getHexaTextChild(block_data, u"block_data");
}

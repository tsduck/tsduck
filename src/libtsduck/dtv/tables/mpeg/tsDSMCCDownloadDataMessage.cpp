//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
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
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::clearContent()
{
    protocol_discriminator = 0x11;
    dsmcc_type = 0x00;
    message_id = 0;
    download_id = 0;
    module_id = 0;
    module_version = 0;
    block_number = 0;
    block_data.clear();
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
    protocol_discriminator = buf.getUInt8();
    dsmcc_type = buf.getUInt8();
    message_id = buf.getUInt16();
    download_id = buf.getUInt32();

    buf.skipBytes(1);

    uint8_t adaptation_length = buf.getUInt8();

    // Skip message length
    buf.skipBytes(2);
    /*uint16_t message_length = buf.getUInt16();*/

    /* For object carousel it should be 0 */
    if (adaptation_length > 0) {
        buf.skipBytes(adaptation_length);
    }

    module_id = buf.getUInt16();
    module_version = buf.getUInt8();

    // Reserved
    buf.skipBytes(1);

    block_number = buf.getUInt16();
    buf.getBytesAppend(block_data);
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------
void ts::DSMCCDownloadDataMessage::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putUInt8(protocol_discriminator);
    buf.putUInt8(dsmcc_type);
    buf.putUInt16(message_id);
    buf.putUInt32(download_id);
    buf.putUInt16(module_id);
    buf.putUInt8(module_version);
    buf.putUInt16(block_number);
    buf.pushState();

    // Loop on new sections until all block bytes are gone.
    size_t block_data_index = 0;
    while (table.sectionCount() == 0 || block_data_index < block_data.size()) {
        block_data_index += buf.putBytes(block_data, block_data_index, std::min(block_data.size() - block_data_index, buf.remainingWriteBytes()));
        addOneSection(table, buf);
    }
}

void ts::DSMCCDownloadDataMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    /*uint16_t message_length = 0;*/
    uint8_t adaptation_length = 0;

    if (buf.canReadBytes(12)) {
        const uint8_t  protocol_discriminator = buf.getUInt8();
        const uint8_t  dsmcc_type = buf.getUInt8();
        const uint16_t message_id = buf.getUInt16();
        const uint32_t download_id = buf.getUInt32();

        // Skip reserved
        buf.skipBytes(1);

        adaptation_length = buf.getUInt8();
        // Skip message length
        /*message_length = buf.getUInt16();*/
        buf.skipBytes(2);

        /* For object carousel it should be 0 */
        if (adaptation_length > 0) {
            buf.skipBytes(adaptation_length);
        }

        disp << margin << UString::Format(u"Protocol discriminator: %n", protocol_discriminator) << std::endl;
        disp << margin << "Dsmcc type: " << DataName(MY_XML_NAME, u"dsmcc_type", dsmcc_type, NamesFlags::HEXA_FIRST) << std::endl;
        if (dsmcc_type == 0x03) {
            disp << margin << "Message id: " << DataName(MY_XML_NAME, u"message_id", message_id, NamesFlags::HEXA_FIRST) << std::endl;
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
    root->setIntAttribute(u"protocol_discriminator", protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", dsmcc_type, true);
    root->setIntAttribute(u"message_id", message_id, true);
    root->setIntAttribute(u"download_id", download_id, true);
    root->setIntAttribute(u"module_id", module_id, true);
    root->setIntAttribute(u"module_version", module_version, true);
    root->addHexaTextChild(u"block_data", block_data, true);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCDownloadDataMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
           element->getIntAttribute(dsmcc_type, u"dsmcc_type", true, 0x03) &&
           element->getIntAttribute(message_id, u"message_id", true) &&
           element->getIntAttribute(download_id, u"download_id", true) &&
           element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_version, u"module_version", true) &&
           element->getHexaTextChild(block_data, u"block_data");
}

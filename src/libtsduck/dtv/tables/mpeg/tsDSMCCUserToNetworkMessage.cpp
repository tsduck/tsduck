//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCUserToNetworkMessage.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSMCC_user_to_network_message"
#define MY_CLASS ts::DSMCCUserToNetworkMessage
#define MY_TID ts::TID_DSMCC_UNM
#define MY_STD ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    modules(this)
{
}

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(DuckContext& duck, const BinaryTable& table) :
    DSMCCUserToNetworkMessage(0, true)
{
    deserialize(duck, table);
}

ts::DSMCCUserToNetworkMessage::Module::Module(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// DSM-CC Message Header
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::MessageHeader::clear()
{
    protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;
    dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;
    message_id = 0;
    transaction_id = 0;
}

void ts::DSMCCUserToNetworkMessage::clearContent()
{
    // DSM-CC Message Header
    header.clear();
    compatibility_descriptor.clear();

    // DSI
    server_id.clear();
    ior.clear();

    // DII
    download_id = 0;
    block_size = 0;
    modules.clear();
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::DSMCCUserToNetworkMessage::isPrivate() const
{
    // According to ISO/IEC 13818-6, section 9.2.2, in all DSM-CC sections, "the private_indicator field
    // shall be set to the complement of the section_syntax_indicator value". For long sections, the
    // syntax indicator is always 1 and, therefore, the private indicator shall always be 0 ("non-private").
    return false;
}

size_t ts::DSMCCUserToNetworkMessage::maxPayloadSize() const
{
    // Although declared as a "non-private section" in the MPEG sense, the
    // DSM-CC section can use up to 4096 bytes according to
    // ETSI TS 102 809 V1.3.1 (2017-06), Table B.2.
    //
    // The maximum section length is 4096 bytes for all types of sections used in object carousel.
    // The section overhead is 12 bytes, leaving a maxium 4084 of payload per section.
    return MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
}

uint16_t ts::DSMCCUserToNetworkMessage::tableIdExtension() const
{
    return uint16_t(header.transaction_id & 0xFFFF);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::deserializePayload(PSIBuffer& buf, const Section& section)
{
    header.protocol_discriminator = buf.getUInt8();
    header.dsmcc_type = buf.getUInt8();
    header.message_id = buf.getUInt16();
    header.transaction_id = buf.getUInt32();

    buf.skipBytes(1);

    const uint8_t adaptation_length = buf.getUInt8();

    buf.skipBytes(2);  // message_length

    // For object carousel it should be 0
    if (adaptation_length > 0) {
        buf.setUserError();
        buf.skipBytes(adaptation_length);
    }

    if (header.message_id == DSMCC_MSGID_DSI) {

        buf.getBytes(server_id, DSMCC_SERVER_ID_SIZE);
        compatibility_descriptor.deserialize(buf);

        // Private_data_length
        buf.pushReadSizeFromLength(16);  // private_data_length

        ior.deserialize(buf);

        buf.skipBytes(4);  // download_taps_count + service_context_list_count + user_info_length

        buf.popState();
    }
    else if (header.message_id == DSMCC_MSGID_DII) {

        download_id = buf.getUInt32();
        block_size = buf.getUInt16();

        buf.skipBytes(10);  // windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        compatibility_descriptor.deserialize(buf);

        const uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {
            Module& module(modules.newEntry());

            module.module_id = buf.getUInt16();
            module.module_size = buf.getUInt32();
            module.module_version = buf.getUInt8();

            buf.skipBytes(1);  // module_info_length

            module.module_timeout = buf.getUInt32();
            module.block_timeout = buf.getUInt32();
            module.min_block_time = buf.getUInt32();

            const uint8_t taps_count = buf.getUInt8();

            for (size_t j = 0; j < taps_count; j++) {
                DSMCCTap& tap(module.taps.emplace_back());
                tap.deserialize(buf);
            }

            uint8_t user_info_length = buf.getUInt8();

            buf.getDescriptorList(module.descs, user_info_length);
        }

        uint16_t private_data_length = buf.getUInt16();

        buf.skipBytes(private_data_length);
    }
    else {
        buf.setUserError();
        buf.skipBytes(buf.remainingReadBytes());
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // DSMCC_UNM Table consist only one section so we do not need to worry about overflow

    buf.putUInt8(header.protocol_discriminator);
    buf.putUInt8(header.dsmcc_type);
    buf.putUInt16(header.message_id);
    buf.putUInt32(header.transaction_id);
    buf.putUInt8(0xFF);  // reserved
    buf.putUInt8(0x00);  // adaptation_length

    buf.pushWriteSequenceWithLeadingLength(16);

    if (header.message_id == DSMCC_MSGID_DSI) {
        buf.putBytes(server_id);
        compatibility_descriptor.serialize(buf, true);

        buf.pushWriteSequenceWithLeadingLength(16);  // private_data

        // IOP::IOR
        ior.serialize(buf);

        buf.putUInt8(0x00);     // download_taps_count
        buf.putUInt8(0x00);     // service_context_list_count
        buf.putUInt16(0x0000);  // user_info_length

        buf.popState();  // close private_data
    }
    else if (header.message_id == DSMCC_MSGID_DII) {

        buf.putUInt32(download_id);
        buf.putUInt16(block_size);

        // ETSI TR 101 202 V1.2.1 5.7.5.1
        // Not used and set to zero
        buf.putUInt8(0x00);         // windowSize
        buf.putUInt8(0x00);         // ackPeriod
        buf.putUInt32(0x00000000);  // tCDownloadWindow
        buf.putUInt32(0x00000000);  // tCDownloadScenario
        compatibility_descriptor.serialize(buf, true);

        buf.putUInt16(uint16_t(modules.size()));

        for (const auto& module : modules) {

            buf.putUInt16(module.second.module_id);
            buf.putUInt32(module.second.module_size);
            buf.putUInt8(module.second.module_version);

            buf.pushWriteSequenceWithLeadingLength(8);  // module_info_length

            // BIOP::ModuleInfo

            buf.putUInt32(module.second.module_timeout);
            buf.putUInt32(module.second.block_timeout);
            buf.putUInt32(module.second.min_block_time);

            buf.putUInt8(uint8_t(module.second.taps.size()));  // taps_count

            for (const auto& tap : module.second.taps) {
                tap.serialize(buf);
            }

            buf.pushWriteSequenceWithLeadingLength(8);  // user_info_length

            // Normaly I would use putDescriptorListWithLength
            // but UserInfoLength is only 1 byte instead of 2 assumed
            // by above method
            buf.putDescriptorList(module.second.descs);

            buf.popState();  // close user_info_length

            buf.popState();  // close module_info_length
        }

        buf.putUInt16(0x0000);  // private_data_length
    }
    buf.popState();  // close message_length
}


//----------------------------------------------------------------------------
// Display one section
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    uint8_t adaptation_length = 0;
    uint16_t message_id = 0;

    if (buf.canReadBytes(MESSAGE_HEADER_SIZE)) {
        const uint8_t protocol_discriminator = buf.getUInt8();
        const uint8_t dsmcc_type = buf.getUInt8();
        message_id = buf.getUInt16();
        const uint32_t transaction_id = buf.getUInt32();

        buf.skipBytes(1);  // reserved

        adaptation_length = buf.getUInt8();

        buf.skipBytes(2);  // message_length

        // For object carousel it should be 0
        if (adaptation_length > 0) {
            buf.skipBytes(adaptation_length);
        }

        disp << margin << UString::Format(u"Protocol discriminator: %n", protocol_discriminator) << std::endl;
        disp << margin << "Dsmcc type: " << DataName(MY_XML_NAME, u"dsmcc_type", dsmcc_type, NamesFlags::HEX_VALUE_NAME) << std::endl;
        if (dsmcc_type == 0x03) {
            disp << margin << "Message id: " << DataName(MY_XML_NAME, u"message_id", message_id, NamesFlags::HEX_VALUE_NAME) << std::endl;
        }
        else {
            disp << margin << UString::Format(u"Message id: %n", message_id) << std::endl;
        }
        disp << margin << UString::Format(u"Transaction id: %n", transaction_id) << std::endl;
    }

    if (message_id == DSMCC_MSGID_DSI) {  // DSI
                                          //
        disp.displayPrivateData(u"Server id", buf, DSMCC_SERVER_ID_SIZE, margin);

        DSMCCCompatibilityDescriptor::Display(disp, buf, margin);
        buf.pushReadSizeFromLength(16);  //private_data_length

        DSMCCIOR::Display(disp, buf, margin);

        uint8_t download_taps_count = buf.getUInt8();
        uint8_t service_context_list_count = buf.getUInt8();
        uint16_t user_info_length = buf.getUInt16();

        disp << margin << UString::Format(u"Download taps count: %n", download_taps_count) << std::endl;
        disp << margin << UString::Format(u"Service context list count: %n", service_context_list_count) << std::endl;
        disp << margin << UString::Format(u"User info length: %n", user_info_length) << std::endl;

        buf.popState();
    }
    else if (message_id == DSMCC_MSGID_DII) {  //DII
                                               //
        disp << margin << UString::Format(u"Download id: %n", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"Block size: %n", buf.getUInt16()) << std::endl;

        buf.skipBytes(10);  // windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        DSMCCCompatibilityDescriptor::Display(disp, buf, margin);

        uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {

            uint16_t module_id = buf.getUInt16();
            uint32_t module_size = buf.getUInt32();
            uint8_t module_version = buf.getUInt8();

            disp << margin << UString::Format(u"Module id: %n", module_id) << std::endl;
            disp << margin << UString::Format(u"Module size: %n", module_size) << std::endl;
            disp << margin << UString::Format(u"Module version: %n", module_version) << std::endl;

            buf.skipBytes(1);  // module_info_length

            uint32_t module_timeout = buf.getUInt32();
            uint32_t block_timeout = buf.getUInt32();
            uint32_t min_block_time = buf.getUInt32();
            uint8_t taps_count = buf.getUInt8();

            disp << margin << UString::Format(u"Module timeout: %n", module_timeout) << std::endl;
            disp << margin << UString::Format(u"Block timeout: %n", block_timeout) << std::endl;
            disp << margin << UString::Format(u"Min block time: %n", min_block_time) << std::endl;
            disp << margin << UString::Format(u"Taps count: %n", taps_count) << std::endl;

            bool ok = true;
            for (size_t k = 0; ok && k < taps_count; k++) {
                ok = DSMCCTap::Display(disp, buf, margin);
            }

            uint8_t user_info_length = buf.getUInt8();

            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
            disp.displayDescriptorList(section, context, false, buf, margin, u"Descriptor List:", u"None", user_info_length);
        }

        uint16_t private_data_length = buf.getUInt16();
        disp.displayPrivateData(u"Private data", buf, private_data_length, margin);
    }
    else {
        buf.setUserError();
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"protocol_discriminator", header.protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", header.dsmcc_type, true);
    root->setIntAttribute(u"message_id", header.message_id, true);
    root->setIntAttribute(u"transaction_id", header.transaction_id, true);

    if (header.message_id == DSMCC_MSGID_DSI) {
        xml::Element* dsi = root->addElement(u"DSI");
        dsi->addHexaTextChild(u"server_id", server_id, true);
        compatibility_descriptor.toXML(duck, dsi, true);

        ior.toXML(duck, dsi);
    }
    else if (header.message_id == DSMCC_MSGID_DII) {

        xml::Element* dii = root->addElement(u"DII");
        dii->setIntAttribute(u"download_id", download_id, true);
        dii->setIntAttribute(u"block_size", block_size, true);
        compatibility_descriptor.toXML(duck, dii, true);

        for (const auto& it : modules) {
            xml::Element* mod = dii->addElement(u"module");
            mod->setIntAttribute(u"module_id", it.second.module_id, true);
            mod->setIntAttribute(u"module_size", it.second.module_size, true);
            mod->setIntAttribute(u"module_version", it.second.module_version, true);
            mod->setIntAttribute(u"module_timeout", it.second.module_timeout, true);
            mod->setIntAttribute(u"block_timeout", it.second.block_timeout, true);
            mod->setIntAttribute(u"min_block_time", it.second.min_block_time, true);

            for (const auto& tap : it.second.taps) {
                tap.toXML(duck, mod);
            }
            it.second.descs.toXML(duck, mod);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCUserToNetworkMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getIntAttribute(header.protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
        element->getIntAttribute(header.dsmcc_type, u"dsmcc_type", true, 0x03) &&
        element->getIntAttribute(header.message_id, u"message_id", true) &&
        element->getIntAttribute(header.transaction_id, u"transaction_id", true);

    if (header.message_id == DSMCC_MSGID_DSI) {

        const xml::Element* dsi_element = element->findFirstChild(u"DSI", true);
        if (dsi_element == nullptr) {
            return false;
        }

        ok = dsi_element->getHexaTextChild(server_id, u"server_id") &&
            compatibility_descriptor.fromXML(duck, dsi_element, false);

        const xml::Element* ior_element = dsi_element->findFirstChild(u"IOR", true);
        if (!ok || ior_element == nullptr) {
            return false;
        }

        ok = ior.fromXML(duck, ior_element);
    }
    else if (header.message_id == DSMCC_MSGID_DII) {
        const xml::Element* dii_element = element->findFirstChild(u"DII", true);
        if (dii_element == nullptr) {
            return false;
        }

        ok = dii_element->getIntAttribute(download_id, u"download_id", true) &&
            dii_element->getIntAttribute(block_size, u"block_size", true) &&
            compatibility_descriptor.fromXML(duck, dii_element, false);

        for (auto& xmod : dii_element->children(u"module", &ok)) {
            auto& module(modules.newEntry());
            ok = xmod.getIntAttribute(module.module_id, u"module_id", true) &&
                xmod.getIntAttribute(module.module_size, u"module_size", true) &&
                xmod.getIntAttribute(module.module_version, u"module_version", true) &&
                xmod.getIntAttribute(module.module_timeout, u"module_timeout", true) &&
                xmod.getIntAttribute(module.block_timeout, u"block_timeout", true) &&
                xmod.getIntAttribute(module.min_block_time, u"min_block_time", true) &&
                module.descs.fromXML(duck, &xmod, u"tap");

            for (auto& xtap : xmod.children(u"tap", &ok)) {
                auto& tap(module.taps.emplace_back());
                ok = tap.fromXML(duck, &xtap, nullptr);
            }
        }
    }
    else {
        // Unknown message_id, nothing to analyze
        ok = false;
    }

    return ok;
}

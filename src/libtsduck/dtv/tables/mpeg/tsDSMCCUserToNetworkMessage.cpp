//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2024, Tristan Claverie
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
#define MY_CLASS    ts::DSMCCUserToNetworkMessage
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur)
{
}

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(DuckContext& duck, const BinaryTable& table) :
    DSMCCUserToNetworkMessage(0, true)
{
    deserialize(duck, table);
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::clearContent()
{
    protocol_discriminator = 0x11;
    dsmcc_type = 0x03;
    message_id = 0;
    transaction_id = 0;

    //DSI
    server_id.clear();
    private_data.clear();

    //DII
    download_id = 0;
    block_size = 0;
    modules.clear();
    dii_private_data.clear();
}

uint16_t ts::DSMCCUserToNetworkMessage::tableIdExtension() const
{
    return 0x0000;
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::deserializePayload(PSIBuffer& buf, const Section& section)
{
    protocol_discriminator = buf.getUInt8();
    dsmcc_type = buf.getUInt8();
    message_id = buf.getUInt16();
    transaction_id = buf.getUInt32();

    buf.skipBytes(1);

    const uint8_t adaptation_length = buf.getUInt8();

    // Skip message length
    buf.skipBytes(2);
    /*uint16_t message_length = buf.getUInt16();*/

    /* For object carousel it should be 0 */
    if (adaptation_length > 0) {
        buf.setUserError();
        buf.skipBytes(adaptation_length);
    }

    if (message_id == 0x1006) {
        buf.getBytes(server_id, SERVER_ID_SIZE);

        // CompatibilityDescriptor
        buf.skipBytes(2);

        const uint16_t private_data_length = buf.getUInt16();

        buf.getBytes(private_data, private_data_length);
    }
    else if (message_id == 0x1002) {
        download_id = buf.getUInt32();
        block_size = buf.getUInt16();

        //windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        buf.skipBytes(10);

        // CompatibilityDescriptor
        buf.skipBytes(2);

        const uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {
            Module module;
            module.module_id = buf.getUInt16();
            module.module_size = buf.getUInt32();
            module.module_version = buf.getUInt8();

            const uint8_t module_info_length = buf.getUInt8();

            buf.getBytes(module.module_info, module_info_length);
            modules.push_back(module);
        }

        const uint16_t private_data_length = buf.getUInt16();

        buf.getBytes(private_data, private_data_length);
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
    buf.putUInt8(protocol_discriminator);
    buf.putUInt8(dsmcc_type);
    buf.putUInt16(message_id);
    buf.putUInt32(transaction_id);

    if (message_id == 0x1006) {
        buf.putBytes(server_id);

        // According to ETSI TR 101 202 V1.2.1 (2003-01), 4.6.5, Table 4.1a
        // DSI and DII messages have only one section.
        buf.putBytes(private_data);
    }
    else if (message_id == 0x1002) {
        buf.putUInt32(download_id);
        buf.putUInt16(block_size);
        buf.putUInt16(modules.size());

        for (const auto& module : modules) {
            buf.putUInt16(module.module_id);
            buf.putUInt32(module.module_size);
            buf.putUInt8(module.module_version);
            buf.putBytes(module.module_info);
        }

        buf.putBytes(dii_private_data);
    }
}

void ts::DSMCCUserToNetworkMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    uint8_t  adaptation_length = 0;
    uint16_t message_id = 0;

    if (buf.canReadBytes(12)) {
        const uint8_t protocol_discriminator = buf.getUInt8();
        const uint8_t dsmcc_type = buf.getUInt8();
        message_id = buf.getUInt16();
        const uint32_t transaction_id = buf.getUInt32();

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
        disp << margin << UString::Format(u"Transaction id: %n", transaction_id) << std::endl;
    }

    if (message_id == 0x1006) {  //DSI
        disp.displayPrivateData(u"Server id", buf, SERVER_ID_SIZE, margin);

        // CompatibilityDescriptor
        buf.skipBytes(2);

        /*uint16_t private_data_length = buf.getUInt16();*/
        buf.skipBytes(2);
        uint32_t  type_id_length = buf.getUInt32();
        ByteBlock type_id {};

        for (size_t i = 0; i < type_id_length; i++) {
            type_id.appendUInt8(buf.getUInt8());
        }

        disp.displayVector(u"Type id: ", type_id, margin);

        uint32_t tagged_profiles_count = buf.getUInt32();

        for (size_t i = 0; i < tagged_profiles_count; i++) {

            uint32_t profile_id_tag = buf.getUInt32();
            uint32_t profile_data_length = buf.getUInt32();
            uint8_t  profile_data_byte_order = buf.getUInt8();

            disp << margin << "ProfileId Tag: " << DataName(MY_XML_NAME, u"tag", profile_id_tag, NamesFlags::HEXA_FIRST) << std::endl;
            disp << margin << UString::Format(u"Profile Data Byte Orded: %n", profile_data_byte_order) << std::endl;

            if (profile_id_tag == 0x49534F06) {  // TAG_BIOP (BIOP Profile Body)
                uint8_t lite_component_count = buf.getUInt8();
                disp << margin << UString::Format(u"Lite Component Count: %n", lite_component_count) << std::endl;

                for (size_t j = 0; j < lite_component_count; j++) {

                    uint32_t componentid_tag = buf.getUInt32();
                    uint8_t  component_data_length = buf.getUInt8();

                    disp << margin << "ComponentId Tag: " << DataName(MY_XML_NAME, u"tag", componentid_tag, NamesFlags::HEXA_FIRST) << std::endl;

                    switch (componentid_tag) {
                        case 0x49534F50: {  // TAG_ObjectLocation

                            uint32_t  carousel_id = buf.getUInt32();
                            uint16_t  module_id = buf.getUInt16();
                            uint8_t   version_major = buf.getUInt8();
                            uint8_t   version_minor = buf.getUInt8();
                            uint8_t   object_key_length = buf.getUInt8();
                            ByteBlock object_key_data {};

                            for (size_t k = 0; k < object_key_length; k++) {
                                object_key_data.appendUInt8(buf.getUInt8());
                            }

                            disp << margin << UString::Format(u"Carousel Id: %n", carousel_id) << std::endl;
                            disp << margin << UString::Format(u"Module Id: %n", module_id) << std::endl;
                            disp << margin << UString::Format(u"Version Major: %n", version_major) << std::endl;
                            disp << margin << UString::Format(u"Version Minor: %n", version_minor) << std::endl;

                            disp.displayVector(u"Object Key Data: ", object_key_data, margin);

                            break;
                        }

                        case 0x49534F40: {  // TAG_ConnBinder

                            uint8_t taps_count = buf.getUInt8();

                            for (size_t k = 0; k < taps_count; k++) {

                                uint16_t id = buf.getUInt16();
                                uint16_t use = buf.getUInt16();
                                uint16_t association_tag = buf.getUInt16();

                                // selector_length
                                buf.skipBytes(1);

                                uint16_t selector_type = buf.getUInt16();
                                uint32_t transaction_id = buf.getUInt32();
                                uint32_t timeout = buf.getUInt32();

                                disp << margin << UString::Format(u"Tap id: %n", id) << std::endl;
                                disp << margin << "Tap use: " << DataName(MY_XML_NAME, u"tap_use", use, NamesFlags::HEXA_FIRST) << std::endl;
                                disp << margin << UString::Format(u"Tap association tag: %n", association_tag) << std::endl;
                                disp << margin << UString::Format(u"Tap selector type: %n", selector_type) << std::endl;
                                disp << margin << UString::Format(u"Tap transaction id: %n", transaction_id) << std::endl;
                                disp << margin << UString::Format(u"Tap timeout: %n", timeout) << std::endl;
                            }
                            break;
                        }

                        default: {
                            disp.displayPrivateData(u"Lite Component Data", buf, component_data_length, margin);
                            break;
                        }
                    }
                }
            }
            else if (profile_id_tag == 0x49534F05) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                disp.displayPrivateData(u"Lite Options Profile Body Data", buf, profile_data_length - 1, margin);
            }
            else {
                disp.displayPrivateData(u"Unknown Profile Data", buf, profile_data_length - 1, margin);
            }
        }

        uint8_t  download_taps_count = buf.getUInt8();
        uint8_t  service_context_list_count = buf.getUInt8();
        uint16_t user_info_length = buf.getUInt16();

        disp << margin << UString::Format(u"Download taps count: %n", download_taps_count) << std::endl;
        disp << margin << UString::Format(u"Service context list count: %n", service_context_list_count) << std::endl;
        disp << margin << UString::Format(u"User info length: %n", user_info_length) << std::endl;
    }
    else if (message_id == 0x1002) {  //DII
        disp << margin << UString::Format(u"Download id: %n", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"Block size: %n", buf.getUInt16()) << std::endl;

        //windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        buf.skipBytes(10);

        // CompatibilityDescriptor
        buf.skipBytes(2);

        uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {

            uint16_t module_id = buf.getUInt16();
            uint32_t module_size = buf.getUInt32();
            uint8_t  module_version = buf.getUInt8();

            disp << margin << UString::Format(u"Module id: %n", module_id) << std::endl;
            disp << margin << UString::Format(u"Module size: %n", module_size) << std::endl;
            disp << margin << UString::Format(u"Module version: %n", module_version) << std::endl;

            /*uint8_t module_info_length = buf.getUInt8();*/
            buf.skipBytes(1);

            uint32_t module_timeout = buf.getUInt32();
            uint32_t block_timeout = buf.getUInt32();
            uint32_t min_block_time = buf.getUInt32();
            uint8_t  taps_count = buf.getUInt8();

            disp << margin << UString::Format(u"Module timeout: %n", module_timeout) << std::endl;
            disp << margin << UString::Format(u"Block timeout: %n", block_timeout) << std::endl;
            disp << margin << UString::Format(u"Min block time: %n", min_block_time) << std::endl;
            disp << margin << UString::Format(u"Taps count: %n", taps_count) << std::endl;

            for (size_t k = 0; k < taps_count; k++) {

                uint16_t  id = buf.getUInt16();
                uint16_t  use = buf.getUInt16();
                uint16_t  association_tag = buf.getUInt16();
                uint8_t   selector_length = buf.getUInt8();
                ByteBlock selector_type {};

                for (size_t l = 0; l < selector_length; l++) {
                    selector_type.appendUInt8(buf.getUInt8());
                }

                disp << margin << UString::Format(u"Tap id: %n", id) << std::endl;
                disp << margin << "Tap use: " << DataName(MY_XML_NAME, u"tap_use", use, NamesFlags::HEXA_FIRST) << std::endl;
                disp << margin << UString::Format(u"Tap association tag: %n", association_tag) << std::endl;
                if (selector_length > 0) {
                    disp.displayVector(u"Selector type: ", selector_type, margin);
                }
            }

            uint8_t user_info_length = buf.getUInt8();

            disp.displayDescriptorList(section, buf, margin, u"Descriptor List:", u"None", user_info_length);
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
    root->setIntAttribute(u"protocol_discriminator", protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", dsmcc_type, true);
    root->setIntAttribute(u"message_id", message_id, true);
    root->setIntAttribute(u"transaction_id", transaction_id, true);

    if (message_id == 0x1006) {
        xml::Element* dsi = root->addElement(u"DSI");
        dsi->addHexaTextChild(u"server_id", server_id, true);
        dsi->addHexaTextChild(u"private_data", private_data, true);
    }
    else if (message_id == 0x1002) {
        xml::Element* dii = root->addElement(u"DII");
        dii->setIntAttribute(u"download_id", download_id, true);
        dii->setIntAttribute(u"block_size", block_size, true);
        dii->setIntAttribute(u"number_of_modules", modules.size(), true);

        for (const auto& module : modules) {
            xml::Element* mod = dii->addElement(u"module");
            mod->setIntAttribute(u"module_id", module.module_id, true);
            mod->setIntAttribute(u"module_size", module.module_size, true);
            mod->setIntAttribute(u"module_version", module.module_version, true);
            mod->addHexaTextChild(u"module_info", module.module_info, true);
        }

        dii->addHexaTextChild(u"dii_private_data", private_data, true);
    }
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCUserToNetworkMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
        element->getIntAttribute(dsmcc_type, u"dsmcc_type", true, 0x03) &&
        element->getIntAttribute(message_id, u"message_id", true) &&
        element->getIntAttribute(transaction_id, u"transaction_id", true);

    if (message_id == 0x1006) {
        ok = element->getHexaTextChild(server_id, u"server_id") &&
             element->getHexaTextChild(private_data, u"private_data");
    }
    else if (message_id == 0x1002) {
        ok = element->getIntAttribute(download_id, u"download_id", true) &&
             element->getIntAttribute(block_size, u"block_size", true);

        xml::ElementVector children;
        ok = element->getChildren(children, u"module");

        for (size_t index = 0; ok && index < children.size(); ++index) {
            Module module;
            ok = children[index]->getIntAttribute(module.module_id, u"module_id", true) &&
                 children[index]->getIntAttribute(module.module_size, u"module_size", true) &&
                 children[index]->getIntAttribute(module.module_version, u"module_version", true) &&
                 children[index]->getHexaTextChild(module.module_info, u"module_info");
            if (ok) {
                modules.push_back(module);
            }
        }
        ok = element->getHexaTextChild(private_data, u"dii_private_data");
    }

    return ok;
}

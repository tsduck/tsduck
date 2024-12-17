//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
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

    //DII
    download_id = 0;
    block_size = 0;
    modules.clear();
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

        // private_data_length
        buf.skipBytes(2);

        uint32_t type_id_length = buf.getUInt32();

        for (size_t i = 0; i < type_id_length; i++) {
            ior.type_id.appendUInt8(buf.getUInt8());
        }

        // CDR alligment rule
        if (type_id_length % 4 != 0) {
            buf.skipBytes(4 - (type_id_length % 4));
        }

        const uint32_t tagged_profiles_count = buf.getUInt32();

        for (size_t i = 0; i < tagged_profiles_count; i++) {
            TaggedProfile tagged_profile;

            tagged_profile.profile_id_tag = buf.getUInt32();

            const uint32_t profile_data_length = buf.getUInt32();

            tagged_profile.profile_data_byte_order = buf.getUInt8();

            if (tagged_profile.profile_id_tag == 0x49534F06) {  // TAG_BIOP (BIOP Profile Body)
                const uint8_t lite_component_count = buf.getUInt8();

                for (size_t j = 0; j < lite_component_count; j++) {

                    const uint32_t component_id_tag = buf.getUInt32();
                    const uint8_t  component_data_length = buf.getUInt8();

                    switch (component_id_tag) {
                        case 0x49534F50: {  // TAG_ObjectLocation

                            LiteComponent biopObjectLocation;

                            biopObjectLocation.component_id_tag = component_id_tag;
                            biopObjectLocation.carousel_id = buf.getUInt32();
                            biopObjectLocation.module_id = buf.getUInt16();
                            biopObjectLocation.version_major = buf.getUInt8();
                            biopObjectLocation.version_minor = buf.getUInt8();
                            const uint8_t object_key_length = buf.getUInt8();

                            for (size_t k = 0; k < object_key_length; k++) {
                                biopObjectLocation.object_key_data.appendUInt8(buf.getUInt8());
                            }

                            tagged_profile.liteComponents.push_back(biopObjectLocation);

                            break;
                        }

                        case 0x49534F40: {  // TAG_ConnBinder

                            LiteComponent dsmConnBinder;

                            dsmConnBinder.component_id_tag = component_id_tag;

                            buf.skipBytes(1);  // taps_count

                            dsmConnBinder.tap.id = buf.getUInt16();
                            dsmConnBinder.tap.use = buf.getUInt16();
                            dsmConnBinder.tap.association_tag = buf.getUInt16();

                            buf.skipBytes(1);  // selector_length

                            dsmConnBinder.tap.selector_type = buf.getUInt16();
                            dsmConnBinder.tap.transaction_id = buf.getUInt32();
                            dsmConnBinder.tap.timeout = buf.getUInt32();

                            for (int n = 0; n < component_data_length - 18; n++) {
                                buf.skipBytes(1);  // selector_data
                            }

                            tagged_profile.liteComponents.push_back(dsmConnBinder);

                            break;
                        }

                        default: {
                            LiteComponent unknownComponent;

                            unknownComponent.component_id_tag = component_id_tag;

                            buf.getBytes(unknownComponent.component_data, component_data_length);

                            tagged_profile.liteComponents.push_back(unknownComponent);

                            break;
                        }
                    }
                }
            }
            else if (tagged_profile.profile_id_tag == 0x49534F05) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                buf.getBytes(tagged_profile.profile_data, profile_data_length - 1);
            }
            else {
                buf.getBytes(tagged_profile.profile_data, profile_data_length - 1);
            }

            ior.tagged_profiles.push_back(tagged_profile);
        }

        // download_taps_count + service_context_list_count + user_info_length
        buf.skipBytes(4);
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
            Module& module(modules.newEntry());

            module.module_id = buf.getUInt16();
            module.module_size = buf.getUInt32();
            module.module_version = buf.getUInt8();

            // moduleInfoLength
            buf.skipBytes(1);

            module.module_timeout = buf.getUInt32();
            module.block_timeout = buf.getUInt32();
            module.min_block_time = buf.getUInt32();

            const uint8_t taps_count = buf.getUInt8();

            /*buf.skipBytes(taps_count * 7);  // reserved*/

            for (size_t j = 0; j < taps_count; j++) {
                Tap tap;

                tap.id = buf.getUInt16();
                tap.use = buf.getUInt16();
                tap.association_tag = buf.getUInt16();

                buf.skipBytes(1);  // selector_length

                module.taps.push_back(tap);
            }

            uint8_t user_info_length = buf.getUInt8();
            /*buf.skipBytes(user_info_length);  // user_info*/
            buf.getDescriptorList(module.descs, user_info_length);
        }

        /*buf.skipBytes(2);  // private_data_length*/
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
    buf.putUInt8(protocol_discriminator);
    buf.putUInt8(dsmcc_type);
    buf.putUInt16(message_id);
    buf.putUInt32(transaction_id);

    if (message_id == 0x1006) {
        buf.putBytes(server_id);
        buf.putBytes(ior.type_id);

        for (auto it = ior.tagged_profiles.begin(); it != ior.tagged_profiles.end();) {
            const TaggedProfile& tagged_profile(*it);

            buf.putUInt32(tagged_profile.profile_id_tag);
            buf.putUInt8(tagged_profile.profile_data_byte_order);

            if (tagged_profile.profile_id_tag == 0x49534F06) {
                for (const auto& liteComponent : tagged_profile.liteComponents) {
                    buf.putUInt32(liteComponent.component_id_tag);

                    switch (liteComponent.component_id_tag) {
                        case 0x49534F50: {  // TAG_ObjectLocation
                            buf.putUInt32(liteComponent.component_id_tag);
                            buf.putUInt32(liteComponent.carousel_id);
                            buf.putUInt16(liteComponent.module_id);
                            buf.putUInt8(liteComponent.version_major);
                            buf.putUInt8(liteComponent.version_minor);
                            buf.putUInt8(liteComponent.object_key_data.size());
                            buf.putBytes(liteComponent.object_key_data);
                            /*}*/
                            break;
                        }

                        case 0x49534F40: {  // TAG_ConnBinder
                            buf.putUInt32(liteComponent.component_id_tag);
                            buf.putUInt16(liteComponent.tap.id);
                            buf.putUInt16(liteComponent.tap.use);
                            buf.putUInt16(liteComponent.tap.association_tag);
                            buf.putUInt16(liteComponent.tap.selector_type);
                            buf.putUInt32(liteComponent.tap.transaction_id);
                            buf.putUInt32(liteComponent.tap.timeout);
                            break;
                        }

                        default: {  //UnknownComponent
                            buf.putUInt32(liteComponent.component_id_tag);
                            buf.putBytes(liteComponent.component_data);
                        }
                    }
                }
            }
            else if (tagged_profile.profile_id_tag == 0x49534F05) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                buf.putBytes(tagged_profile.profile_data);
            }
            else {
                buf.putBytes(tagged_profile.profile_data);
            }
        }
    }
    else if (message_id == 0x1002) {
        buf.putUInt32(download_id);
        buf.putUInt16(block_size);
        buf.putUInt16(modules.size());

        for (auto it_module = modules.begin(); !buf.error() && it_module != modules.end();) {
            const Module& module(it_module->second);

            buf.putUInt16(module.module_id);
            buf.putUInt32(module.module_size);
            buf.putUInt8(module.module_version);

            buf.putUInt32(module.module_timeout);
            buf.putUInt32(module.block_timeout);
            buf.putUInt32(module.min_block_time);

            for (const auto& tap : module.taps) {
                buf.putUInt16(tap.id);
                buf.putUInt16(tap.use);
                buf.putUInt16(tap.association_tag);
                buf.putUInt8(0);  // selector_length
            }
            buf.putDescriptorList(module.descs);
        }
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

            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards());
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
    root->setIntAttribute(u"protocol_discriminator", protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", dsmcc_type, true);
    root->setIntAttribute(u"message_id", message_id, true);
    root->setIntAttribute(u"transaction_id", transaction_id, true);

    if (message_id == 0x1006) {
        xml::Element* dsi = root->addElement(u"DSI");
        dsi->addHexaTextChild(u"server_id", server_id, true);

        xml::Element* ior_entry = dsi->addElement(u"IOR");
        ior_entry->addHexaTextChild(u"type_id", ior.type_id, true);

        for (const auto& profile : ior.tagged_profiles) {
            xml::Element* profile_entry = ior_entry->addElement(u"tagged_profile");
            profile_entry->setIntAttribute(u"profile_id_tag", profile.profile_id_tag, true);
            profile_entry->setIntAttribute(u"profile_data_byte_order", profile.profile_data_byte_order, true);

            if (profile.profile_id_tag == 0x49534F06) {

                xml::Element* biop_profile_body_entry = profile_entry->addElement(u"BIOP_Profile_Body");
                biop_profile_body_entry->setIntAttribute(u"lite_component_count", profile.liteComponents.size(), false);

                for (const auto& liteComponent : profile.liteComponents) {

                    switch (liteComponent.component_id_tag) {
                        case 0x49534F50: {  // TAG_ObjectLocation
                            xml::Element* biop_object_location_entry = biop_profile_body_entry->addElement(u"BIOP_object_location");
                            biop_object_location_entry->setIntAttribute(u"component_id_tag", liteComponent.component_id_tag, true);
                            biop_object_location_entry->setIntAttribute(u"carousel_id", liteComponent.carousel_id, true);
                            biop_object_location_entry->setIntAttribute(u"module_id", liteComponent.module_id, true);
                            biop_object_location_entry->setIntAttribute(u"version_major", liteComponent.version_major, true);
                            biop_object_location_entry->setIntAttribute(u"version_minor", liteComponent.version_minor, true);
                            biop_object_location_entry->addHexaTextChild(u"object_key_data", liteComponent.object_key_data, true);
                            break;
                        }

                        case 0x49534F40: {  // TAG_ConnBinder
                            xml::Element* dsm_conn_binder_entry = profile_entry->addElement(u"DSM_conn_binder");
                            dsm_conn_binder_entry->setIntAttribute(u"component_id_tag", liteComponent.component_id_tag, true);

                            xml::Element* tap_entry = dsm_conn_binder_entry->addElement(u"BIOP_tap");
                            tap_entry->setIntAttribute(u"id", liteComponent.tap.id, true);
                            tap_entry->setIntAttribute(u"use", liteComponent.tap.use, true);
                            tap_entry->setIntAttribute(u"association_tag", liteComponent.tap.association_tag, true);
                            tap_entry->setIntAttribute(u"selector_type", liteComponent.tap.selector_type, true);
                            tap_entry->setIntAttribute(u"transaction_id", liteComponent.tap.transaction_id, true);
                            tap_entry->setIntAttribute(u"timeout", liteComponent.tap.timeout, true);
                            break;
                        }

                        default: {
                            xml::Element* unknown_component_entry = profile_entry->addElement(u"Unknown_component");
                            unknown_component_entry->setIntAttribute(u"component_id_tag", liteComponent.component_id_tag, true);
                            unknown_component_entry->addHexaTextChild(u"component_data", liteComponent.component_data, true);
                        }
                    }
                }
            }
            else if (profile.profile_id_tag == 0x49534F05) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                profile_entry->addHexaTextChild(u"profile_data", profile.profile_data, true);
            }
            else {
                profile_entry->addHexaTextChild(u"profile_data", profile.profile_data, true);
            }
        }
    }
    else if (message_id == 0x1002) {

        xml::Element* dii = root->addElement(u"DII");
        dii->setIntAttribute(u"download_id", download_id, true);
        dii->setIntAttribute(u"block_size", block_size, true);
        dii->setIntAttribute(u"number_of_modules", modules.size(), false);

        for (const auto& it : modules) {
            xml::Element* mod = dii->addElement(u"module");
            mod->setIntAttribute(u"module_id", it.second.module_id, true);
            mod->setIntAttribute(u"module_size", it.second.module_size, true);
            mod->setIntAttribute(u"module_version", it.second.module_version, true);
            mod->setIntAttribute(u"module_timeout", it.second.module_timeout, true);
            mod->setIntAttribute(u"block_timeout", it.second.block_timeout, true);
            mod->setIntAttribute(u"min_block_time", it.second.min_block_time, true);

            for (const auto& tap : it.second.taps) {
                xml::Element* t = mod->addElement(u"tap");
                t->setIntAttribute(u"id", tap.id, true);
                t->setIntAttribute(u"use", tap.use, true);
                t->setIntAttribute(u"association_tag", tap.association_tag, true);
                t->setIntAttribute(u"selector_type", tap.selector_type, true);
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
        element->getIntAttribute(protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
        element->getIntAttribute(dsmcc_type, u"dsmcc_type", true, 0x03) &&
        element->getIntAttribute(message_id, u"message_id", true) &&
        element->getIntAttribute(transaction_id, u"transaction_id", true);

    if (message_id == 0x1006) {

        ok = element->getHexaTextChild(server_id, u"server_id");

        const xml::Element* ior_element = element->findFirstChild(u"IOR", true);
        xml::ElementVector  children;

        ok = ior_element->getHexaTextChild(ior.type_id, u"type_id") &&
             ior_element->getChildren(children, u"tagged_profile");

        for (auto it = children.begin(); ok && it != children.end(); ++it) {
            TaggedProfile tagged_profile;

            ok = (*it)->getIntAttribute(tagged_profile.profile_id_tag, u"profile_id_tag", true) &&
                 (*it)->getIntAttribute(tagged_profile.profile_data_byte_order, u"profile_data_byte_order", true);

            //TODO LiteComponents

            if (ok) {
                ior.tagged_profiles.push_back(tagged_profile);
            }
        }
    }
    else if (message_id == 0x1002) {
        xml::ElementVector children;

        ok = element->getIntAttribute(download_id, u"download_id", true) &&
             element->getIntAttribute(block_size, u"block_size", true) &&
             element->getChildren(children, u"module");

        for (size_t it = 0; ok && it < children.size(); ++it) {
            Module& module(modules.newEntry());

            xml::ElementVector children2;

            ok = children[it]->getIntAttribute(module.module_id, u"module_id", true) &&
                 children[it]->getIntAttribute(module.module_size, u"module_size", true) &&
                 children[it]->getIntAttribute(module.module_version, u"module_version", true) &&
                 children[it]->getIntAttribute(module.module_timeout, u"module_timeout", true) &&
                 children[it]->getIntAttribute(module.block_timeout, u"block_timeout", true) &&
                 children[it]->getIntAttribute(module.min_block_time, u"min_block_time", true) &&
                 children[it]->getChildren(children2, u"tap");


            for (size_t it2 = 0; ok && it2 < children2.size(); ++it2) {
                Tap tap;

                ok = children2[it2]->getIntAttribute(tap.id, u"id", true) &&
                     children2[it2]->getIntAttribute(tap.use, u"use", true) &&
                     children2[it2]->getIntAttribute(tap.association_tag, u"association_tag", true) &&
                     children2[it2]->getIntAttribute(tap.selector_type, u"selector_type", true);

                module.taps.push_back(tap);
            }
            ok = module.descs.fromXML(duck, children[it]);
        }
    }

    return ok;
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCLiteComponent.h"
#include "tsNames.h"

//----------------------------------------------------------------------------
// LiteComponent
//----------------------------------------------------------------------------

void ts::DSMCCLiteComponent::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(component_id_tag);
    buf.pushWriteSequenceWithLeadingLength(8);  //component_data

    switch (component_id_tag) {
        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
            buf.putUInt32(carousel_id);
            buf.putUInt16(module_id);
            buf.putUInt8(version_major);
            buf.putUInt8(version_minor);
            buf.putUInt8(uint8_t(object_key_data.size()));
            buf.putBytes(object_key_data);
            break;
        }

        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
            buf.putUInt8(0x01);        // TODO: for now only one tap assumed but this needs to be fixed
            tap.serialize(buf);
            break;
        }

        default: {  // UnknownComponent
            if (component_data.has_value()) {
                buf.putBytes(component_data.value());
            }
            break;
        }
    }
    buf.popState();
}

void ts::DSMCCLiteComponent::deserialize(PSIBuffer& buf)
{
    component_id_tag = buf.getUInt32();
    buf.pushReadSizeFromLength(8);

    switch (component_id_tag) {
        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
            carousel_id = buf.getUInt32();
            module_id = buf.getUInt16();
            version_major = buf.getUInt8();
            version_minor = buf.getUInt8();
            buf.getBytes(object_key_data, buf.getUInt8());
            break;
        }
        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
            const uint8_t taps_count = buf.getUInt8();
            for (size_t k = 0; k < taps_count; k++) {
                // taps_count is assumed to be 1, rewrite the same tap, need to be fixed
                tap.deserialize(buf);
            }
            break;
        }
        default: {
            component_data = buf.getBytes();
            break;
        }
    }

    buf.popState();
}

void ts::DSMCCLiteComponent::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    const uint32_t component_id_tag = buf.getUInt32();
    buf.pushReadSizeFromLength(8);

    disp << margin << "ComponentId Tag: " << NameFromSection(u"dtv", u"DSMCC_user_to_network_message.tag", component_id_tag, NamesFlags::HEX_VALUE_NAME) << std::endl;

    switch (component_id_tag) {
        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
                                           //
            disp << margin << UString::Format(u"Carousel Id: %n", buf.getUInt32()) << std::endl;
            disp << margin << UString::Format(u"Module Id: %n", buf.getUInt16()) << std::endl;
            disp << margin << UString::Format(u"Version Major: %n", buf.getUInt8()) << std::endl;
            disp << margin << UString::Format(u"Version Minor: %n", buf.getUInt8()) << std::endl;

            ByteBlock object_key {};

            buf.getBytes(object_key, buf.getUInt8());

            disp.displayVector(u"Object Key Data: ", object_key, margin);
            break;
        }
        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder

            const uint8_t taps_count = buf.getUInt8();
            bool ok = true;

            for (size_t i = 0; ok && i < taps_count; i++) {
                ok = DSMCCTap::Display(disp, buf, margin);
            }
            break;
        }
        default: {
            disp.displayPrivateData(u"Lite Component Data", buf, NPOS, margin);
            break;
        }
    }
    buf.popState();
}

void ts::DSMCCLiteComponent::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* lite_component = parent->addElement(u"lite_component");
    lite_component->setIntAttribute(u"component_id_tag", component_id_tag, true);

    switch (component_id_tag) {
        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
            xml::Element* biop_object_location = lite_component->addElement(u"BIOP_object_location");
            biop_object_location->setIntAttribute(u"carousel_id", carousel_id, true);
            biop_object_location->setIntAttribute(u"module_id", module_id, true);
            biop_object_location->setIntAttribute(u"version_major", version_major, true);
            biop_object_location->setIntAttribute(u"version_minor", version_minor, true);
            biop_object_location->addHexaTextChild(u"object_key_data", object_key_data, true);
            break;
        }
        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
            tap.toXML(duck, lite_component->addElement(u"DSM_conn_binder"));
            break;
        }
        default: {
            xml::Element* unknown_component = lite_component->addElement(u"Unknown_component");
            if (component_data.has_value()) {
                unknown_component->addHexaTextChild(u"component_data", component_data.value(), true);
            }
            break;
        }
    }
}

bool ts::DSMCCLiteComponent::fromXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(component_id_tag, u"component_id_tag", true);
    if (!ok)
        return false;

    switch (component_id_tag) {  // TAG_ObjectLocation
        case DSMCC_TAG_OBJECT_LOCATION: {
            const xml::Element* biop_object_location = element->findFirstChild(u"BIOP_object_location", true);
            ok = biop_object_location != nullptr &&
                biop_object_location->getIntAttribute(carousel_id, u"carousel_id", true) &&
                biop_object_location->getIntAttribute(module_id, u"module_id", true) &&
                biop_object_location->getIntAttribute(version_major, u"version_major", true) &&
                biop_object_location->getIntAttribute(version_minor, u"version_minor", true) &&
                biop_object_location->getHexaTextChild(object_key_data, u"object_key_data");
            break;
        }
        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
            const xml::Element* dsm_conn_binder = element->findFirstChild(u"DSM_conn_binder");
            ok = dsm_conn_binder != nullptr && tap.fromXML(duck, dsm_conn_binder);
            break;
        }
        default: {  //UnknownComponent
            const xml::Element* unknown_component = element->findFirstChild(u"Unknown_component");
            ByteBlock temp {};
            if (unknown_component != nullptr && unknown_component->getHexaTextChild(temp, u"component_data")) {
                component_data = temp;
            }
            break;
        }
    }
    return ok;
}

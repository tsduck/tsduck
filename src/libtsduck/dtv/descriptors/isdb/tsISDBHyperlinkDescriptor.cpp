//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBHyperlinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_hyperlink_descriptor"
#define MY_CLASS ts::ISDBHyperlinkDescriptor
#define MY_DID ts::DID_ISDB_HYPERLINK
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

constexpr auto LINK_TO_SERVICE = 0x01;
constexpr auto LINK_TO_EVENT = 0x02;
constexpr auto LINK_TO_MODULE = 0x03;
constexpr auto LINK_TO_CONTENT = 0x04;
constexpr auto LINK_TO_CONTENT_MODULE = 0x05;
constexpr auto LINK_TO_ERT_NODE = 0x06;
constexpr auto LINK_TO_STORED_CONTENT = 0x07;

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBHyperlinkDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBHyperlinkDescriptor::clearContent()
{
    hyper_linkage_type = 0;
    link_destination_type = 0;
    link_to_service.reset();
    link_to_event.reset();
    link_to_module.reset();
    link_to_content.reset();
    link_to_content_module.reset();
    link_to_ert_node.reset();
    link_to_stored_content.reset();
    private_data.clear();
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::ServiceTriplet::clear()
{
    original_network_id = 0;
    transport_stream_id = 0;
    service_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::EventTriplet::clear()
{
    ServiceTriplet::clear();
    event_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::ModuleTriplet::clear()
{
    EventTriplet::clear();
    component_tag = 0;
    module_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::ContentTriplet::clear()
{
    ServiceTriplet::clear();
    content_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::ContentModuleTriplet::clear()
{
    ContentTriplet::clear();
    component_tag = 0;
    module_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::ERTNode::clear()
{
    information_provider_id = 0;
    event_relation_id = 0;
    node_id = 0;
}

void ts::ISDBHyperlinkDescriptor::ISDBHyperlinkDescriptor::StoredContent::clear()
{
    uri.clear();
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBHyperlinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(hyper_linkage_type);
    buf.putUInt8(link_destination_type);
    switch (link_destination_type) {
        case LINK_TO_SERVICE:
            if (link_to_service.has_value()) {
                link_to_service.value().serialize(buf);
            }
            break;
        case LINK_TO_EVENT:
            if (link_to_event.has_value()) {
                link_to_event.value().serialize(buf);
            }
            break;
        case LINK_TO_MODULE:
            if (link_to_module.has_value()) {
                link_to_module.value().serialize(buf);
            }
            break;
        case LINK_TO_CONTENT:
            if (link_to_content.has_value()) {
                link_to_content.value().serialize(buf);
            }
            break;
        case LINK_TO_CONTENT_MODULE:
            if (link_to_content_module.has_value()) {
                link_to_content_module.value().serialize(buf);
            }
            break;
        case LINK_TO_ERT_NODE:
            if (link_to_ert_node.has_value()) {
                link_to_ert_node.value().serialize(buf);
            }
            break;
        case LINK_TO_STORED_CONTENT:
            if (link_to_stored_content.has_value()) {
                link_to_stored_content.value().serialize(buf);
            }
            break;
        default:
            break;
    }
    buf.putBytes(private_data);
}

void ts::ISDBHyperlinkDescriptor::ServiceTriplet::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(original_network_id);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(service_id);
}

void ts::ISDBHyperlinkDescriptor::EventTriplet::serialize(PSIBuffer& buf) const
{
    ServiceTriplet::serialize(buf);
    buf.putUInt16(event_id);
}

void ts::ISDBHyperlinkDescriptor::ModuleTriplet::serialize(PSIBuffer& buf) const
{
    EventTriplet::serialize(buf);
    buf.putUInt8(component_tag);
    buf.putUInt16(module_id);
}

void ts::ISDBHyperlinkDescriptor::ContentTriplet::serialize(PSIBuffer& buf) const
{
    ServiceTriplet::serialize(buf);
    buf.putUInt16(content_id);
}

void ts::ISDBHyperlinkDescriptor::ContentModuleTriplet::serialize(PSIBuffer& buf) const
{
    ContentTriplet::serialize(buf);
    buf.putUInt8(component_tag);
    buf.putUInt16(module_id);
}

void ts::ISDBHyperlinkDescriptor::ERTNode::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(information_provider_id);
    buf.putUInt16(event_relation_id);
    buf.putUInt16(node_id);
}
void ts::ISDBHyperlinkDescriptor::StoredContent::serialize(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(uri);
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBHyperlinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    hyper_linkage_type = buf.getUInt8();
    link_destination_type = buf.getUInt8();
    switch (link_destination_type) {
        case LINK_TO_SERVICE: {
            ServiceTriplet svc(buf);
            link_to_service = svc;
            }
            break;
        case LINK_TO_EVENT: {
            EventTriplet evt(buf);
            link_to_event = evt;
            }
            break;
        case LINK_TO_MODULE: {
            ModuleTriplet mod(buf);
            link_to_module = mod;
            }
            break;
        case LINK_TO_CONTENT: {
            ModuleTriplet mod(buf);
            link_to_module = mod;
            }
            break;
        case LINK_TO_CONTENT_MODULE: {
            ContentModuleTriplet mod(buf);
            link_to_content_module = mod;
            }
            break;
        case LINK_TO_ERT_NODE: {
            ERTNode ert(buf);
            link_to_ert_node = ert;
            }
            break;
        case LINK_TO_STORED_CONTENT: {
            StoredContent uri(buf);
            link_to_stored_content = uri;
            }
            break;
        default:
            break;
    }
    buf.getBytes(private_data);
 }

void ts::ISDBHyperlinkDescriptor::ServiceTriplet::deserialize(PSIBuffer& buf) {
    original_network_id = buf.getUInt16();
    transport_stream_id = buf.getUInt16();
    service_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::EventTriplet::deserialize(PSIBuffer& buf)
{
    ServiceTriplet::deserialize(buf);
    event_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::ModuleTriplet::deserialize(PSIBuffer& buf)
{
    EventTriplet::deserialize(buf);
    component_tag = buf.getUInt8();
    module_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::ContentTriplet::deserialize(PSIBuffer& buf)
{
    ServiceTriplet::deserialize(buf);
    content_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::ContentModuleTriplet::deserialize(PSIBuffer& buf)
{
    ContentTriplet::deserialize(buf);
    component_tag = buf.getUInt8();
    module_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::ERTNode::deserialize(PSIBuffer& buf)
{
    information_provider_id = buf.getUInt16();
    event_relation_id = buf.getUInt16();
    node_id = buf.getUInt16();
}

void ts::ISDBHyperlinkDescriptor::StoredContent::deserialize(PSIBuffer& buf)
{
    buf.getStringWithByteLength(uri);
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBHyperlinkDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
 {
    if (buf.canReadBytes(3)) {
        disp << margin << "Linkage type: " << DataName(MY_XML_NAME, u"hyper_linkage_type", buf.getUInt8());
        uint8_t _destinaton_type = buf.getUInt8();
        disp << ", destination type: " << DataName(MY_XML_NAME, u"link_destination_type", _destinaton_type) << std::endl;
        switch (_destinaton_type) {
            case LINK_TO_SERVICE: {
                ServiceTriplet tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_EVENT: {
                EventTriplet tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_MODULE: {
                ModuleTriplet tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_CONTENT: {
                ContentTriplet tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_CONTENT_MODULE: {
                ContentModuleTriplet tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_ERT_NODE: {
                ERTNode tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            case LINK_TO_STORED_CONTENT: {
                StoredContent tmp;
                tmp.display(disp, buf, margin);
                }
                break;
            default:
                break;
        }
        disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
    }
}

void ts::ISDBHyperlinkDescriptor::ServiceTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::EventTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    ServiceTriplet::display(disp, buf, margin);
    disp << margin << UString::Format(u"Event id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::ModuleTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    EventTriplet::display(disp, buf, margin);
    disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    disp << margin << UString::Format(u"Module id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::ContentTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    ServiceTriplet::display(disp, buf, margin);
    disp << margin << UString::Format(u"Content id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::ContentModuleTriplet::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    ContentTriplet::display(disp, buf, margin);
    disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    disp << margin << UString::Format(u"Module id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::ERTNode::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Information provider id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    disp << margin << UString::Format(u"Event relation id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    disp << margin << UString::Format(u"Node id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
}

void ts::ISDBHyperlinkDescriptor::StoredContent::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "URL: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBHyperlinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"hyper_linkage_type", hyper_linkage_type, true);
    root->setIntAttribute(u"link_destination_type", link_destination_type, true);

    switch (link_destination_type) {
        case LINK_TO_SERVICE: {
            if (link_to_service.has_value()) {
                link_to_service.value().toXML(root->addElement(u"Service"));
            }
        }
        break;
        case LINK_TO_EVENT: {
            if (link_to_event.has_value()) {
                link_to_event.value().toXML(root->addElement(u"Event"));
            }
        }
        break;
        case LINK_TO_MODULE: {
            if (link_to_module.has_value()) {
                link_to_module.value().toXML(root->addElement(u"Module"));
            }
        }
        break;
        case LINK_TO_CONTENT: {
            if (link_to_content.has_value()) {
                link_to_content.value().toXML(root->addElement(u"Content"));
            }
        }
        break;
        case LINK_TO_CONTENT_MODULE: {
            if (link_to_content_module.has_value()) {
                link_to_content_module.value().toXML(root->addElement(u"ContentModule"));
            }
        }
        break;
        case LINK_TO_ERT_NODE: {
            if (link_to_ert_node.has_value()) {
                link_to_ert_node.value().toXML(root->addElement(u"ERTNode"));
            }
        }
        break;
        case LINK_TO_STORED_CONTENT: {
            if (link_to_stored_content.has_value()) {
                link_to_stored_content.value().toXML(root->addElement(u"StoredContent"));
            }
        }
        break;
        default:
        break;
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}

void ts::ISDBHyperlinkDescriptor::ServiceTriplet::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"service_id", service_id, true);
}

void ts::ISDBHyperlinkDescriptor::EventTriplet::toXML(xml::Element* root) const
{
    ServiceTriplet::toXML(root);
    root->setIntAttribute(u"event_id", event_id, true);
}

void ts::ISDBHyperlinkDescriptor::ModuleTriplet::toXML(xml::Element* root) const
{
    EventTriplet::toXML(root);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setIntAttribute(u"module_id", module_id, true);
}

void ts::ISDBHyperlinkDescriptor::ContentTriplet::toXML(xml::Element* root) const
{
    ServiceTriplet::toXML(root);
    root->setIntAttribute(u"content_id", content_id, true);
}

void ts::ISDBHyperlinkDescriptor::ContentModuleTriplet::toXML(xml::Element* root) const
{
    ContentTriplet::toXML(root);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setIntAttribute(u"module_id", module_id, true);
}

void ts::ISDBHyperlinkDescriptor::ERTNode::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"information_provider_id", information_provider_id, true);
    root->setIntAttribute(u"event_relation_id", event_relation_id, true);
    root->setIntAttribute(u"node_id", node_id, true);
}

void ts::ISDBHyperlinkDescriptor::StoredContent::toXML(xml::Element* root) const
{
    root->setAttribute(u"uri", uri);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBHyperlinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(hyper_linkage_type, u"hyper_linkage_type", true) &&
              element->getIntAttribute(link_destination_type, u"link_destination_type", true) &&
              element->getHexaTextChild(private_data, u"private_data", false);

    if (ok) {
        ts::xml::ElementVector elems;
        switch (link_destination_type) {
            case LINK_TO_SERVICE: {
                ServiceTriplet tmp;
                ok = element->getChildren(elems, u"Service", 1, 1) &&
                     tmp.fromXML(elems[0]);
                if (ok) {
                    link_to_service = tmp;
                }
            }
            break;
            case LINK_TO_EVENT: {
                EventTriplet t;
                ok = element->getChildren(elems, u"Event", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_event = t;
                }
            }
            break;
            case LINK_TO_MODULE: {
                ModuleTriplet t;
                ok = element->getChildren(elems, u"Module", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_module = t;
                }
            }
            break;
            case LINK_TO_CONTENT: {
                ContentTriplet t;
                ok = element->getChildren(elems, u"Content", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_content = t;
                }
            }
            break;
            case LINK_TO_CONTENT_MODULE: {
                ContentModuleTriplet t;
                ok = element->getChildren(elems, u"ContentModule", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_content_module = t;
                }
            }
            break;
            case LINK_TO_ERT_NODE: {
                ERTNode t;
                ok = element->getChildren(elems, u"ERTNode", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_ert_node = t;
                }
            }
            break;
            case LINK_TO_STORED_CONTENT: {
                StoredContent t;
                ok = element->getChildren(elems, u"StoredContent", 1, 1) &&
                     t.fromXML(elems[0]);
                if (ok) {
                    link_to_stored_content = t;
                }
            }
            break;
            default:
            break;
        }
    }
    return ok;
}

bool ts::ISDBHyperlinkDescriptor::ServiceTriplet::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(original_network_id, u"original_network_id", true) &&
           element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
           element->getIntAttribute(service_id, u"service_id", true);
}

bool ts::ISDBHyperlinkDescriptor::EventTriplet::fromXML(const xml::Element* element)
{
    return ServiceTriplet::fromXML(element) &&
           element->getIntAttribute(event_id, u"event_id", true);
}

bool ts::ISDBHyperlinkDescriptor::ModuleTriplet::fromXML(const xml::Element* element)
{
    return EventTriplet::fromXML(element) &&
           element->getIntAttribute(component_tag, u"component_tag", true) &&
           element->getIntAttribute(module_id, u"module_id", true);
}

bool ts::ISDBHyperlinkDescriptor::ContentTriplet::fromXML(const xml::Element* element)
{
    return ServiceTriplet::fromXML(element) &&
           element->getIntAttribute(content_id, u"content_id", true);
}

bool ts::ISDBHyperlinkDescriptor::ContentModuleTriplet::fromXML(const xml::Element* element)
{
    return ContentTriplet::fromXML(element) &&
           element->getIntAttribute(component_tag, u"component_tag", true) &&
           element->getIntAttribute(module_id, u"module_id", true);
}

bool ts::ISDBHyperlinkDescriptor::ERTNode::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(information_provider_id, u"information_provider_id", true) &&
           element->getIntAttribute(event_relation_id, u"event_relation_id", true) &&
           element->getIntAttribute(node_id, u"node_id", true);
}

bool ts::ISDBHyperlinkDescriptor::StoredContent::fromXML(const xml::Element* element)
{
    return element->getAttribute(uri, u"uri", true);
}


//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBComponentGroupDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_component_group_descriptor"
#define MY_CLASS    ts::ISDBComponentGroupDescriptor
#define MY_DID      ts::DID_ISDB_COMP_GROUP
#define MY_PDS      ts::PDS_ISDB
#define MY_STD      ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBComponentGroupDescriptor::ISDBComponentGroupDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBComponentGroupDescriptor::ISDBComponentGroupDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBComponentGroupDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBComponentGroupDescriptor::clearContent()
{
    component_group_type = 0;
    components.clear();
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::clear()
{
    component_group_id = 0;
    CA_units.clear();
    total_bit_rate.reset();
    explanation.clear();
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::clear()
{
    CA_unit_id = 0;
    component_tags.clear();
}


//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

bool ts::ISDBComponentGroupDescriptor::matching_total_bit_rate()
{
    size_t count = 0;
    for (auto c : components) {
        if (c.total_bit_rate.has_value()) {
            count++;
        }
    }
    return (count == 0 || count == components.size());
}


bool ts::ISDBComponentGroupDescriptor::total_bit_rate_flag() const
{
    return (components.size() ? components[0].total_bit_rate.has_value() : false);
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBComponentGroupDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(component_group_type, 3);
    bool tbr_flag = total_bit_rate_flag();
    buf.putBit(tbr_flag);
    buf.putBits(components.size(), 4);

    for (auto component : components) {
        component.serialize(buf, tbr_flag);
    }
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::serialize(PSIBuffer& buf, bool total_bit_rate_flag) const
{
    buf.putBits(component_group_id, 4);
    buf.putBits(CA_units.size(), 4);
    for (auto ca_unit : CA_units) {
        ca_unit.serialize(buf);
    }
    if (total_bit_rate_flag) {
        buf.putUInt8(total_bit_rate.value_or(0));
    }
    buf.putStringWithByteLength(explanation);
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::serialize(PSIBuffer& buf) const
{
    buf.putBits(CA_unit_id, 4);
    buf.putBits(component_tags.size(), 4);
    for (auto tag : component_tags) {
        buf.putUInt8(tag);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBComponentGroupDescriptor::deserializePayload(PSIBuffer& buf)
{
    component_group_type = buf.getBits<uint8_t>(3);
    const bool    total_bit_rate_flag = buf.getBool();
    const uint8_t num_components = buf.getBits<uint8_t>(4);
    for (size_t i = 0; i < num_components; i++) {
        ComponentGroup group(buf, total_bit_rate_flag);
        components.push_back(group);
    }
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::deserialize(PSIBuffer& buf, bool total_bit_rate_flag)
{
    component_group_id = buf.getBits<uint8_t>(4);
    const uint8_t num_ca_groups = buf.getBits<uint8_t>(4);
    for (size_t i = 0; i < num_ca_groups; i++) {
        CAUnit ca(buf);
        CA_units.push_back(ca);
    }
    if (total_bit_rate_flag) {
        total_bit_rate = buf.getUInt8();
    }
    buf.getStringWithByteLength(explanation);
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::deserialize(PSIBuffer& buf)
{
    CA_unit_id = buf.getBits<uint8_t>(4);
    const uint8_t num_of_components = buf.getBits<uint8_t>(4);
    for (size_t i = 0; i < num_of_components; i++) {
        component_tags.push_back(buf.getUInt8());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBComponentGroupDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Component group type: " << DataName(MY_XML_NAME, u"component_group_type", buf.getBits<uint8_t>(3), NamesFlags::VALUE) << std::endl;
        const bool total_bit_rate_flag = buf.getBool();
        const size_t num_of_group = buf.getBits<uint8_t>(4);
        for (size_t i = 0; i < num_of_group; i++) {
            ComponentGroup comp;
            comp.display(disp, buf, margin, total_bit_rate_flag, i);
        }
    }
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, bool total_bit_rate_flag, size_t group_num)
{
    disp << margin << UString::Format(u"Component group #%2d; id: ", {group_num})
         << DataName(MY_XML_NAME, u"component_group_id", buf.getBits<uint8_t>(4), NamesFlags::VALUE)  << std::endl;
    const size_t num_of_CA_unit = buf.getBits<uint8_t>(4);
    for (size_t i = 0; i < num_of_CA_unit; i++) {
        CAUnit unit;
        unit.display(disp, buf, margin + u"  ", i);
    }
    if (total_bit_rate_flag) {
        const uint8_t tbr = buf.getUInt8();
        disp << margin << "  "
             << UString::Format(u"Total bit rate: %7.2fMbps (%d)", {double(float(tbr) / 4), tbr}) << std::endl;
    }
    disp << margin << "  "
         << "Explanation: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, size_t group_num)
{
    disp << margin << UString::Format(u"CA unit #%2d", {group_num}) << "; id: "
         << DataName(MY_XML_NAME, u"CA_unit_id", buf.getBits<uint8_t>(4), NamesFlags::VALUE) << std::endl;
    const size_t num_of_component = buf.getBits<uint8_t>(4);
    ByteBlock _component_tags;
    for (size_t i = 0; i < num_of_component; i++) {
        _component_tags.push_back(buf.getUInt8());
    }
    disp.displayVector(u"Component tags:", _component_tags, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBComponentGroupDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"component_group_type", component_group_type, true);
    for (auto component : components) {
        component.toXML(root->addElement(u"component_group"));
    }
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"component_group_id", component_group_id, true);
    root->setOptionalIntAttribute(u"total_bit_rate", total_bit_rate);
    root->setAttribute(u"explanation", explanation, true);
    for (auto unit : CA_units) {
        unit.toXML(root->addElement(u"CAUnit"));
    }
}

void ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"CA_unit_id", CA_unit_id);
    root->addHexaTextChild(u"component_tags", component_tags, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBComponentGroupDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector _components;
    bool ok = element->getIntAttribute(component_group_type, u"component_group_type", true, 0, 0, 0x7) &&
              element->getChildren(_components, u"component_group", 0, 16);
    bool components_ok = true;
    if (ok) {
        for (auto component : _components) {
            ComponentGroup newComponent;
            if (newComponent.fromXML(component)) {
                components.push_back(newComponent);
            }
            else {
                components_ok = false;
            }
        }
        if (components_ok && !matching_total_bit_rate()) {
            element->report().error(u"total_bit_rate must be specified for all or none of the component_group in  <%s>, line %d", {element->name(), element->lineNumber()});
            components_ok = false;
        }
    }
    return ok && components_ok;
}

bool ts::ISDBComponentGroupDescriptor::ComponentGroup::fromXML(const xml::Element* element)
{
    xml::ElementVector CAunits;
    bool ok = element->getIntAttribute(component_group_id, u"component_group_id", true, 0, 0, 0xF) &&
              element->getChildren(CAunits, u"CAUnit", 0, 0xF) &&
              element->getOptionalIntAttribute(total_bit_rate, u"total_bit_rate") &&
              element->getAttribute(explanation, u"explanation", false, u"", 0, 255);

    bool units_ok = true;
    if (ok) {
        for (auto unit : CAunits) {
            CAUnit newCAunit;
            if (newCAunit.fromXML(unit)) {
                CA_units.push_back(newCAunit);
            }
            else {
                units_ok = false;
            }
        }
    }
    return ok && units_ok;

}

bool ts::ISDBComponentGroupDescriptor::ComponentGroup::CAUnit::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(CA_unit_id, u"CA_unit_id", true, 0, 0, 0xF) &&
           element->getHexaTextChild(component_tags, u"component_tags", false, 0, 0xF);
}

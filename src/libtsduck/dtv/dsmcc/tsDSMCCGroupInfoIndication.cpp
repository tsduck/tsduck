//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCGroupInfoIndication.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"


void ts::DSMCCGroupInfoIndication::clear()
{
    groups.clear();
    private_data.clear();
}


void ts::DSMCCGroupInfoIndication::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(uint16_t(groups.size()));
    for (const auto& group : groups) {
        buf.putUInt32(group.group_id);
        buf.putUInt32(group.group_size);
        group.group_compatibility.serialize(buf);
        buf.putUInt16(uint16_t(group.group_info.size()));
        buf.putBytes(group.group_info);
    }
    buf.putUInt16(uint16_t(private_data.size()));
    buf.putBytes(private_data);
}


void ts::DSMCCGroupInfoIndication::deserialize(PSIBuffer& buf)
{
    clear();
    const uint16_t number_of_groups = buf.getUInt16();
    for (size_t i = 0; i < number_of_groups && !buf.error(); ++i) {
        Group& group(groups.emplace_back());
        group.group_id = buf.getUInt32();
        group.group_size = buf.getUInt32();
        group.group_compatibility.deserialize(buf);
        const uint16_t group_info_length = buf.getUInt16();
        buf.getBytes(group.group_info, group_info_length);
    }
    const uint16_t private_data_length = buf.getUInt16();
    buf.getBytes(private_data, private_data_length);
}


void ts::DSMCCGroupInfoIndication::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(2)) {
        return;
    }
    const uint16_t number_of_groups = buf.getUInt16();
    disp << margin << UString::Format(u"Number of groups: %d", number_of_groups) << std::endl;

    for (size_t i = 0; i < number_of_groups && buf.canReadBytes(8); ++i) {
        const uint32_t group_id = buf.getUInt32();
        const uint32_t group_size = buf.getUInt32();
        disp << margin << UString::Format(u"- Group #%d, id: %n, size: %n", i, group_id, group_size) << std::endl;
        DSMCCCompatibilityDescriptor::Display(disp, buf, margin + u"  ");
        if (buf.canReadBytes(2)) {
            const size_t group_info_length = buf.getUInt16();
            disp.displayPrivateData(u"Group info", buf, group_info_length, margin + u"  ");
        }
    }

    if (buf.canReadBytes(2)) {
        const size_t private_data_length = buf.getUInt16();
        disp.displayPrivateData(u"Private data", buf, private_data_length, margin);
    }
}


void ts::DSMCCGroupInfoIndication::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* gii_element = parent->addElement(u"GroupInfoIndication");
    for (const auto& group : groups) {
        xml::Element* xgroup = gii_element->addElement(u"group");
        xgroup->setIntAttribute(u"group_id", group.group_id, true);
        xgroup->setIntAttribute(u"group_size", group.group_size, true);
        group.group_compatibility.toXML(duck, xgroup, true);
        xgroup->addHexaTextChild(u"group_info", group.group_info, true);
    }
    gii_element->addHexaTextChild(u"private_data", private_data, true);
}


bool ts::DSMCCGroupInfoIndication::fromXML(DuckContext& duck, const xml::Element* element)
{
    clear();
    bool ok = true;
    for (auto& xgroup : element->children(u"group", &ok)) {
        Group& group(groups.emplace_back());
        ok = ok &&
             xgroup.getIntAttribute(group.group_id, u"group_id", true) &&
             xgroup.getIntAttribute(group.group_size, u"group_size", true) &&
             group.group_compatibility.fromXML(duck, &xgroup, false) &&
             xgroup.getHexaTextChild(group.group_info, u"group_info", false);
    }
    ok = ok && element->getHexaTextChild(private_data, u"private_data", false);
    return ok;
}

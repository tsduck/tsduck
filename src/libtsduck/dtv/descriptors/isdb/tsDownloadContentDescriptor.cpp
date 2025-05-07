//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDownloadContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"download_content_descriptor"
#define MY_CLASS    ts::DownloadContentDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_DOWNLOAD_CONT, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DownloadContentDescriptor::DownloadContentDescriptor() :
    AbstractDownloadContentDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DownloadContentDescriptor::clearContent()
{
    reboot = false;
    add_on = false;
    component_size = 0;
    download_id = 0;
    time_out_value_DII = 0;
    leak_rate = 0;
    component_tag = 0;
    compatibility_descriptor.clear();
    module_info.clear();
    private_data.clear();
    text_info.reset();
}

ts::DownloadContentDescriptor::DownloadContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    DownloadContentDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DownloadContentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(reboot);
    buf.putBit(add_on);
    buf.putBit(!compatibility_descriptor.empty());
    buf.putBit(!module_info.empty());
    buf.putBit(text_info.has_value());
    buf.putReserved(3);
    buf.putUInt32(component_size);
    buf.putUInt32(download_id);
    buf.putUInt32(time_out_value_DII);
    buf.putBits(leak_rate, 22);
    buf.putReserved(2);
    buf.putUInt8(component_tag);
    if (!compatibility_descriptor.empty()) {
        compatibility_descriptor.serialize(buf);
    }
    if (!module_info.empty()) {
        buf.putUInt16(uint16_t(module_info.size()));
        for (const auto& module : module_info) {
            module.serializePayload(buf);
        }
    }
    buf.putUInt8(uint8_t(private_data.size()));
    buf.putBytes(private_data);
    if (text_info.has_value()) {
        text_info.value().serializePayload(buf);
    }
}

void ts::DownloadContentDescriptor::deserializePayload(PSIBuffer& buf)
{
    reboot = buf.getBool();
    add_on = buf.getBool();
    const bool compatibility_flag = buf.getBool();
    const bool module_info_flag = buf.getBool();
    const bool text_info_flag = buf.getBool();
    buf.skipReservedBits(3);
    component_size = buf.getUInt32();
    download_id = buf.getUInt32();
    time_out_value_DII = buf.getUInt32();
    buf.getBits(leak_rate, 22);
    buf.skipReservedBits(2);
    component_tag = buf.getUInt8();
    if (compatibility_flag) {
        compatibility_descriptor.deserialize(buf);
    }
    if (module_info_flag) {
        for (size_t count = buf.getUInt16(); count > 0; --count) {
            module_info.emplace_back();
            module_info.back().deserializePayload(buf);
        }
    }
    buf.getBytes(private_data, buf.getUInt8());
    if (text_info_flag) {
        text_info.emplace();
        text_info.value().deserializePayload(buf);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DownloadContentDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(17)) {
        disp << margin << "Reboot: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Add-on: " << UString::TrueFalse(buf.getBool()) << std::endl;
        const bool compatibility_flag = buf.getBool();
        const bool module_info_flag = buf.getBool();
        const bool text_info_flag = buf.getBool();
        buf.skipReservedBits(3);
        disp << margin << "Component size: " << buf.getUInt32() << " bytes" << std::endl;
        disp << margin << UString::Format(u"Download id: %n", buf.getUInt32()) << std::endl;
        disp << margin << "Timeout DII: " << buf.getUInt32() << std::endl;
        disp << margin << "Leak rate: " << buf.getBits<uint32_t>(22) << " bytes" << std::endl;
        buf.skipReservedBits(2);
        disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
        bool ok = true;
        if (compatibility_flag) {
            ok = DSMCCCompatibilityDescriptor::Display(disp, buf, margin);
        }
        if (ok && module_info_flag) {
            ok = buf.canReadBytes(2);
            if (ok) {
                const size_t count = buf.getUInt16();
                disp << margin << "Number of modules: " << count << std::endl;
                for (size_t i = 0; ok && i < count; ++i) {
                    disp << margin << "- Module #" << i << std::endl;
                    ok = Module::Display(disp, buf, margin + u"  ");
                }
            }
        }
        if (ok) {
            ok = buf.canReadBytes(1);
            if (ok) {
                const size_t count = buf.getUInt8();
                ok = buf.canReadBytes(count);
                disp.displayPrivateData(u"Private data", buf, count, margin);
            }
        }
        if (ok && text_info_flag) {
            disp << margin << "Text info:" << std::endl;
            TextInfo::Display(disp, buf, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DownloadContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"reboot", reboot);
    root->setBoolAttribute(u"add_on", add_on);
    root->setIntAttribute(u"component_size", component_size);
    root->setIntAttribute(u"download_id", download_id, true);
    root->setIntAttribute(u"time_out_value_DII", time_out_value_DII);
    root->setIntAttribute(u"leak_rate", leak_rate);
    root->setIntAttribute(u"component_tag", component_tag, true);
    compatibility_descriptor.toXML(duck, root, true);
    for (const auto& module : module_info) {
        module.buildXML(duck, root);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
    if (text_info.has_value()) {
        text_info.value().buildXML(duck, root);
    }
}

bool ts::DownloadContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtext, xmods;
    bool ok =
        element->getBoolAttribute(reboot, u"reboot", true) &&
        element->getBoolAttribute(add_on, u"add_on", true) &&
        element->getIntAttribute(component_size, u"component_size", true) &&
        element->getIntAttribute(download_id, u"download_id", true) &&
        element->getIntAttribute(time_out_value_DII, u"time_out_value_DII", true) &&
        element->getIntAttribute(leak_rate, u"leak_rate", true, 0, 0, 0x003FFFFF) &&
        element->getIntAttribute(component_tag, u"component_tag", true) &&
        compatibility_descriptor.fromXML(duck, element, false) &&
        element->getChildren(xmods, u"module") &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(xtext, u"text_info", 0, 1);

    for (size_t i = 0; ok && i < xmods.size(); ++i) {
        module_info.emplace_back();
        ok = module_info.back().analyzeXML(duck, xmods[i]);
    }
    if (ok && !xtext.empty()) {
        text_info.emplace();
        ok = text_info.value().analyzeXML(duck, xtext[0]);
    }
    return ok;
}

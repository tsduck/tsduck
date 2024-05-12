//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
#define MY_CLASS ts::DownloadContentDescriptor
#define MY_DID ts::DID_ISDB_DOWNLOAD_CONT
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DownloadContentDescriptor::DownloadContentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
        compatibility_descriptor.serializePayload(buf);
    }
    if (!module_info.empty()) {
        module_info.serializePayload(buf);
    }
    buf.putUInt8(uint8_t(private_data.size()));
    buf.putBytes(private_data);
    if (text_info.has_value()) {
        text_info.value().serializePayload(buf);
    }
}

void ts::DownloadContentDescriptor::ContentSubdescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(type);
    buf.putUInt8(uint8_t(additional_information.size()));
    buf.putBytes(additional_information);
}

void ts::DownloadContentDescriptor::ContentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(descriptor_type);
    buf.pushWriteSequenceWithLeadingLength(8);
    buf.putUInt8(specifier_type);
    buf.putUInt24(specifier_data);
    buf.putUInt16(model);
    buf.putUInt16(version);
    buf.putUInt8(uint8_t(subdescs.size()));
    for (const auto& subd : subdescs) {
        subd.serializePayload(buf);
    }
    buf.popState();
}

void ts::DownloadContentDescriptor::CompatibilityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.pushWriteSequenceWithLeadingLength(16);
    buf.putUInt16(uint16_t(size()));
    for (const auto& desc : *this) {
        desc.serializePayload(buf);
    }
    buf.popState();
}

void ts::DownloadContentDescriptor::Module::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(module_id);
    buf.putUInt32(module_size);
    buf.putUInt8(uint8_t(module_info.size()));
    buf.putBytes(module_info);
}

void ts::DownloadContentDescriptor::ModuleInfo::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(uint16_t(size()));
    for (const auto& module : *this) {
        module.serializePayload(buf);
    }
}

void ts::DownloadContentDescriptor::TextInfo::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

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
        compatibility_descriptor.deserializePayload(buf);
    }
    if (module_info_flag) {
        module_info.deserializePayload(buf);
    }
    buf.getBytes(private_data, buf.getUInt8());
    if (text_info_flag) {
        text_info.emplace();
        text_info.value().deserializePayload(buf);
    }
}

void ts::DownloadContentDescriptor::ContentSubdescriptor::deserializePayload(PSIBuffer& buf)
{
    type = buf.getUInt8();
    buf.getBytes(additional_information, buf.getUInt8());
}

void ts::DownloadContentDescriptor::ContentDescriptor::deserializePayload(PSIBuffer& buf)
{
    descriptor_type = buf.getUInt8();
    buf.pushReadSizeFromLength(8);
    specifier_type = buf.getUInt8();
    specifier_data = buf.getUInt24();
    model = buf.getUInt16();
    version = buf.getUInt16();
    for (size_t subcount = buf.getUInt8(); subcount > 0; --subcount) {
        subdescs.emplace_back();
        subdescs.back().deserializePayload(buf);
    }
    buf.popState();
}

void ts::DownloadContentDescriptor::CompatibilityDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(16);
    for (size_t subcount = buf.getUInt16(); subcount > 0; --subcount) {
        emplace_back();
        back().deserializePayload(buf);
    }
    buf.popState();
}

void ts::DownloadContentDescriptor::Module::deserializePayload(PSIBuffer& buf)
{
    module_id = buf.getUInt16();
    module_size = buf.getUInt32();
    buf.getBytes(module_info, buf.getUInt8());
}

void ts::DownloadContentDescriptor::ModuleInfo::deserializePayload(PSIBuffer& buf)
{
    for (size_t count = buf.getUInt16(); count > 0; --count) {
        emplace_back();
        back().deserializePayload(buf);
    }
}

void ts::DownloadContentDescriptor::TextInfo::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DownloadContentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
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
            ok = CompatibilityDescriptor::Display(disp, buf, margin);
        }
        if (ok && module_info_flag) {
            ok = ModuleInfo::Display(disp, buf, margin);
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

bool ts::DownloadContentDescriptor::ContentSubdescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(2);
    if (ok) {
        disp << margin << UString::Format(u"Type: %n", buf.getUInt8()) << std::endl;
        const size_t count = buf.getUInt8();
        ok = buf.canReadBytes(count);
        disp.displayPrivateData(u"Additional info", buf, count, margin);
    }
    return ok;
}

bool ts::DownloadContentDescriptor::ContentDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(11);
    if (ok) {
        disp << margin << UString::Format(u"Descriptor type: %n", buf.getUInt8());
        const size_t length = buf.getUInt8();
        ok = buf.canReadBytes(length);
        disp << ", size: " << length << " bytes" << std::endl;
        disp << margin << UString::Format(u"Specifier type: %n", buf.getUInt8());
        disp << UString::Format(u", data: %n", buf.getUInt24()) << std::endl;
        disp << margin << UString::Format(u"Model: %n", buf.getUInt16());
        disp << UString::Format(u", version: %d", buf.getUInt16()) << std::endl;
        const size_t count = buf.getUInt8();
        for (size_t i = 0; ok && i < count; ++i) {
            disp << margin << "- Subdescriptor #" << i << std::endl;
            ok = ContentSubdescriptor::Display(disp, buf, margin + u"  ");
        }
    }
    return ok;
}

bool ts::DownloadContentDescriptor::CompatibilityDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(4);
    if (ok) {
        buf.pushReadSizeFromLength(16);
        const size_t count = buf.getUInt16();
        disp << margin << "Compatibility descriptor (" << count << " descriptors)" << std::endl;
        for (size_t i = 0; ok && i < count; ++i) {
            disp << margin << "- Descriptor #" << i << std::endl;
            ok = ContentDescriptor::Display(disp, buf, margin + u"  ");
        }
        buf.popState();
    }
    return ok;
}

bool ts::DownloadContentDescriptor::Module::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(7);
    if (ok) {
        disp << margin << UString::Format(u"Module id: %n", buf.getUInt16());
        disp << UString::Format(u", size: %'d bytes", buf.getUInt32()) << std::endl;
        const size_t count = buf.getUInt8();
        ok = buf.canReadBytes(count);
        disp.displayPrivateData(u"Module info", buf, count, margin);
    }
    return ok;
}

bool ts::DownloadContentDescriptor::ModuleInfo::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(2);
    if (ok) {
        const size_t count = buf.getUInt16();
        disp << margin << "Number of modules: " << count << std::endl;
        for (size_t i = 0; ok && i < count; ++i) {
            disp << margin << "- Module #" << i << std::endl;
            ok = Module::Display(disp, buf, margin + u"  ");
        }
    }
    return ok;
}

bool ts::DownloadContentDescriptor::TextInfo::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(4);
    if (ok) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
    return ok;
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
    if (!compatibility_descriptor.empty()) {
        compatibility_descriptor.buildXML(duck, root);
    }
    module_info.buildXML(duck, root);
    root->addHexaTextChild(u"private_data", private_data, true);
    if (text_info.has_value()) {
        text_info.value().buildXML(duck, root);
    }
}

void ts::DownloadContentDescriptor::ContentSubdescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"subdescriptor");
    e->setIntAttribute(u"type", type, true);
    e->addHexaText(additional_information, true);
}

void ts::DownloadContentDescriptor::ContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"descriptor");
    e->setIntAttribute(u"descriptor_type", descriptor_type, true);
    e->setIntAttribute(u"specifier_type", specifier_type, true);
    e->setIntAttribute(u"specifier_data", specifier_data, true);
    e->setIntAttribute(u"model", model, true);
    e->setIntAttribute(u"version", version);
    for (const auto& sub : subdescs) {
        sub.buildXML(duck, e);
    }
}

void ts::DownloadContentDescriptor::CompatibilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"compatibility_descriptor");
    for (const auto& desc : *this) {
        desc.buildXML(duck, e);
    }
}

void ts::DownloadContentDescriptor::Module::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"module");
    e->setIntAttribute(u"module_id", module_id, true);
    e->setIntAttribute(u"module_size", module_size);
    e->addHexaTextChild(u"module_info", module_info, true);
}

void ts::DownloadContentDescriptor::ModuleInfo::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& module : *this) {
        module.buildXML(duck, root);
    }
}

void ts::DownloadContentDescriptor::TextInfo::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"text_info");
    e->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    e->setAttribute(u"text", text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DownloadContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtext;
    bool ok =
        element->getBoolAttribute(reboot, u"reboot", true) &&
        element->getBoolAttribute(add_on, u"add_on", true) &&
        element->getIntAttribute(component_size, u"component_size", true) &&
        element->getIntAttribute(download_id, u"download_id", true) &&
        element->getIntAttribute(time_out_value_DII, u"time_out_value_DII", true) &&
        element->getIntAttribute(leak_rate, u"leak_rate", true, 0, 0, 0x003FFFFF) &&
        element->getIntAttribute(component_tag, u"component_tag", true) &&
        compatibility_descriptor.analyzeXML(duck, element) &&
        module_info.analyzeXML(duck, element) &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(xtext, u"text_info", 0, 1);
    if (ok && !xtext.empty()) {
        text_info.emplace();
        ok = text_info.value().analyzeXML(duck, xtext[0]);
    }
    return ok;
}

bool ts::DownloadContentDescriptor::ContentSubdescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(type, u"type", true) &&
           element->getHexaText(additional_information);
}

bool ts::DownloadContentDescriptor::ContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xsub;
    bool ok =
        element->getIntAttribute(descriptor_type, u"descriptor_type", true) &&
        element->getIntAttribute(specifier_type, u"specifier_type", true) &&
        element->getIntAttribute(specifier_data, u"specifier_data", true, 0, 0, 0x00FFFFFF) &&
        element->getIntAttribute(model, u"model", true) &&
        element->getIntAttribute(version, u"version", true) &&
        element->getChildren(xsub, u"subdescriptor");
    for (size_t i = 0; ok && i < xsub.size(); ++i) {
        subdescs.emplace_back();
        ok = subdescs.back().analyzeXML(duck, xsub[i]);
    }
    return ok;
}

bool ts::DownloadContentDescriptor::CompatibilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* parent)
{
    xml::ElementVector xcompat;
    bool ok = parent->getChildren(xcompat, u"compatibility_descriptor", 0, 1);
    if (ok && !xcompat.empty()) {
        xml::ElementVector xdesc;
        ok = xcompat[0]->getChildren(xdesc, u"descriptor");
        for (size_t i = 0; ok && i < xdesc.size(); ++i) {
            emplace_back();
            ok = back().analyzeXML(duck, xdesc[i]);
        }
    }
    return ok;
}

bool ts::DownloadContentDescriptor::Module::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_size, u"module_size", true) &&
           element->getHexaTextChild(module_info, u"module_info");
}

bool ts::DownloadContentDescriptor::ModuleInfo::analyzeXML(DuckContext& duck, const xml::Element* parent)
{
    xml::ElementVector xmods;
    bool ok = parent->getChildren(xmods, u"module");
    for (size_t i = 0; ok && i < xmods.size(); ++i) {
        emplace_back();
        ok = back().analyzeXML(duck, xmods[i]);
    }
    return ok;
}

bool ts::DownloadContentDescriptor::TextInfo::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getAttribute(text, u"text", true);
}

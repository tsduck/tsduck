//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDownloadContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractDownloadContentDescriptor::~AbstractDownloadContentDescriptor()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractDownloadContentDescriptor::ContentSubdescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(type);
    buf.putUInt8(uint8_t(additional_information.size()));
    buf.putBytes(additional_information);
}

void ts::AbstractDownloadContentDescriptor::ContentDescriptor::serializePayload(PSIBuffer& buf) const
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

void ts::AbstractDownloadContentDescriptor::CompatibilityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.pushWriteSequenceWithLeadingLength(16);
    buf.putUInt16(uint16_t(descs.size()));
    for (const auto& desc : descs) {
        desc.serializePayload(buf);
    }
    buf.popState();
}

void ts::AbstractDownloadContentDescriptor::Module::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(module_id);
    buf.putUInt32(module_size);
    buf.putUInt8(uint8_t(module_info.size()));
    buf.putBytes(module_info);
}

void ts::AbstractDownloadContentDescriptor::TextInfo::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractDownloadContentDescriptor::ContentSubdescriptor::deserializePayload(PSIBuffer& buf)
{
    type = buf.getUInt8();
    buf.getBytes(additional_information, buf.getUInt8());
}

void ts::AbstractDownloadContentDescriptor::ContentDescriptor::deserializePayload(PSIBuffer& buf)
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

void ts::AbstractDownloadContentDescriptor::CompatibilityDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(16);
    for (size_t subcount = buf.getUInt16(); subcount > 0; --subcount) {
        descs.emplace_back();
        descs.back().deserializePayload(buf);
    }
    buf.popState();
}

void ts::AbstractDownloadContentDescriptor::Module::deserializePayload(PSIBuffer& buf)
{
    module_id = buf.getUInt16();
    module_size = buf.getUInt32();
    buf.getBytes(module_info, buf.getUInt8());
}

void ts::AbstractDownloadContentDescriptor::TextInfo::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

bool ts::AbstractDownloadContentDescriptor::ContentSubdescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
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

bool ts::AbstractDownloadContentDescriptor::ContentDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
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

bool ts::AbstractDownloadContentDescriptor::CompatibilityDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
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

bool ts::AbstractDownloadContentDescriptor::Module::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
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

bool ts::AbstractDownloadContentDescriptor::TextInfo::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
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

void ts::AbstractDownloadContentDescriptor::ContentSubdescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"subdescriptor");
    e->setIntAttribute(u"type", type, true);
    e->addHexaText(additional_information, true);
}

void ts::AbstractDownloadContentDescriptor::ContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
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

void ts::AbstractDownloadContentDescriptor::CompatibilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"compatibility_descriptor");
    for (const auto& desc : descs) {
        desc.buildXML(duck, e);
    }
}

void ts::AbstractDownloadContentDescriptor::Module::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"module");
    e->setIntAttribute(u"module_id", module_id, true);
    e->setIntAttribute(u"module_size", module_size);
    e->addHexaTextChild(u"module_info", module_info, true);
}

void ts::AbstractDownloadContentDescriptor::TextInfo::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"text_info");
    e->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    e->setAttribute(u"text", text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractDownloadContentDescriptor::ContentSubdescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(type, u"type", true) &&
           element->getHexaText(additional_information);
}

bool ts::AbstractDownloadContentDescriptor::ContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
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

bool ts::AbstractDownloadContentDescriptor::CompatibilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* parent)
{
    xml::ElementVector xcompat;
    bool ok = parent->getChildren(xcompat, u"compatibility_descriptor", 0, 1);
    if (ok && !xcompat.empty()) {
        xml::ElementVector xdesc;
        ok = xcompat[0]->getChildren(xdesc, u"descriptor");
        for (size_t i = 0; ok && i < xdesc.size(); ++i) {
            descs.emplace_back();
            ok = descs.back().analyzeXML(duck, xdesc[i]);
        }
    }
    return ok;
}

bool ts::AbstractDownloadContentDescriptor::Module::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_size, u"module_size", true) &&
           element->getHexaTextChild(module_info, u"module_info");
}

bool ts::AbstractDownloadContentDescriptor::TextInfo::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getAttribute(text, u"text", true);
}

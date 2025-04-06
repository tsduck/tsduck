//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDFIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DFIT"
#define MY_CLASS ts::DFIT
#define MY_TID ts::TID_DFIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DFIT::DFIT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_)
{
}

ts::DFIT::DFIT(DuckContext& duck, const BinaryTable& table) :
    DFIT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::DFIT::tableIdExtension() const
{
    return uint16_t((font_id_extension << 7) | (font_id & 0x7F));
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DFIT::clearContent()
{
    font_id_extension = 0;
    font_id = 0;
    font_style_weight.clear();
    font_file_URI.clear();
    font_size.clear();
    font_family.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DFIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    font_id = uint8_t(section.tableIdExtension() & 0x7F);
    font_id_extension = section.tableIdExtension() >> 7;

    // Loop on resolution providers.
    while (buf.canReadBytes(1)) {
        const uint8_t type = buf.getUInt8();
        switch (type) {
            case 0x00: {
                FontStyleWeight sw;
                buf.getBits(sw.font_style, 3);
                buf.getBits(sw.font_weight, 4);
                buf.skipReservedBits(1, 0);
                font_style_weight.push_back(sw);
                break;
            }
            case 0x01: {
                FontFile ff;
                buf.skipReservedBits(4, 0);
                buf.getBits(ff.font_file_format, 4);
                ff.uri = buf.getUTF8WithLength();
                font_file_URI.push_back(ff);
                break;
            }
            case 0x02: {
                font_size.push_back(buf.getUInt16());
                break;
            }
            case 0x03: {
                font_family = buf.getUTF8WithLength();
                break;
            }
            default: {
                buf.setUserError();
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DFIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // In theory, several sections are allowed. However, each table describes one font only
    // and all data will fit into one section. Therefore, we do not care about creating more than
    // one section. If a font description does not fit into one section, the serialization fails.

    for (const auto& it : font_style_weight) {
        buf.putUInt8(0x00);
        buf.putBits(it.font_style, 3);
        buf.putBits(it.font_weight, 4);
        buf.putReservedZero(1);
    }
    for (const auto& it : font_file_URI) {
        buf.putUInt8(0x01);
        buf.putReservedZero(4);
        buf.putBits(it.font_file_format, 4);
        buf.putUTF8WithLength(it.uri);
    }
    for (const auto& it : font_size) {
        buf.putUInt8(0x02);
        buf.putUInt16(it);
    }
    buf.putUInt8(0x03);
    buf.putUTF8WithLength(font_family);
}


//----------------------------------------------------------------------------
// A static method to display a DFIT section.
//----------------------------------------------------------------------------

void ts::DFIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Font id: %n, id extension: %n", uint8_t(section.tableIdExtension() & 0x7F), uint16_t(section.tableIdExtension() >> 7)) << std::endl;

    while (buf.canReadBytes(1)) {
        const uint8_t type = buf.getUInt8();
        disp << margin << "- Font info type: " << DataName(MY_XML_NAME, u"font_info_type", type, NamesFlags::HEX_VALUE_NAME) << std::endl;
        switch (type) {
            case 0x00: {
                disp << margin << "  Font style: " << DataName(MY_XML_NAME, u"font_style", buf.getBits<uint8_t>(3), NamesFlags::HEX_VALUE_NAME);
                disp << ", font weight: " << DataName(MY_XML_NAME, u"font_weight", buf.getBits<uint8_t>(4), NamesFlags::HEX_VALUE_NAME) << std::endl;
                buf.skipReservedBits(1, 0);
                break;
            }
            case 0x01: {
                buf.skipReservedBits(4, 0);
                disp << margin << "  Font file format: " << DataName(MY_XML_NAME, u"font_file_format", buf.getBits<uint8_t>(4), NamesFlags::HEX_VALUE_NAME) << std::endl;
                disp << margin << "  Font file URI: \"" << buf.getUTF8WithLength() <<"\"" << std::endl;
                break;
            }
            case 0x02: {
                disp << margin << "  Font size: " << buf.getUInt16() << " pixels" << std::endl;
                break;
            }
            case 0x03: {
                disp << margin << "  Font family: \"" << buf.getUTF8WithLength() <<"\"" << std::endl;
                break;
            }
            default: {
                return;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DFIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"font_id", font_id, true);
    root->setIntAttribute(u"font_id_extension", font_id_extension, true);
    root->setAttribute(u"font_family", font_family);
    for (const auto& it : font_style_weight) {
        xml::Element* e = root->addElement(u"font_style_weight");
        e->setIntAttribute(u"font_style", it.font_style);
        e->setIntAttribute(u"font_weight", it.font_weight);
    }
    for (const auto& it : font_file_URI) {
        xml::Element* e = root->addElement(u"font_file_URI");
        e->setIntAttribute(u"font_file_format", it.font_file_format);
        e->setAttribute(u"uri", it.uri);
    }
    for (const auto& it : font_size) {
        root->addElement(u"font_size")->setIntAttribute(u"font_size", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DFIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xstyle, xfile, xsize;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getIntAttribute(font_id, u"font_id", true, 0, 0, 0x7F) &&
        element->getIntAttribute(font_id_extension, u"font_id_extension", false, 0, 0, 0x01FF) &&
        element->getAttribute(font_family, u"font_family", true) &&
        element->getChildren(xstyle, u"font_style_weight", 1) &&
        element->getChildren(xfile, u"font_file_URI", 1) &&
        element->getChildren(xsize, u"font_size");

    for (const auto& it : xstyle) {
        FontStyleWeight sw;
        ok = it->getIntAttribute(sw.font_style, u"font_style", true, 0, 0, 7) &&
             it->getIntAttribute(sw.font_weight, u"font_weight", true, 0, 0, 15) &&
             ok;
        font_style_weight.push_back(sw);
    }
    for (const auto& it : xfile) {
        FontFile ff;
        ok = it->getIntAttribute(ff.font_file_format, u"font_file_format", true, 0, 0, 15) &&
             it->getAttribute(ff.uri, u"uri", true) &&
             ok;
        font_file_URI.push_back(ff);
    }
    for (const auto& it : xsize) {
        uint16_t size = 0;
        ok = it->getIntAttribute(size, u"font_size", true) && ok;
        font_size.push_back(size);
    }
    return ok;
}

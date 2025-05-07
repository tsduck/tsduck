//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCTap.h"
#include "tsDSMCC.h"
#include "tsNames.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Total number of bytes to serialize the Tap().
//----------------------------------------------------------------------------

size_t ts::DSMCCTap::binarySize() const
{
    if (selector_type == DSMCC_TAPSELTYPE_MESSAGE) {
        return 17;
    }
    else if (selector_type.has_value()) {
        return 9 + selector_bytes.size();
    }
    else {
        return 7;
    }
}


//----------------------------------------------------------------------------
// Clear the content of the Tap() structure.
//----------------------------------------------------------------------------

void ts::DSMCCTap::clear()
{
    id = 0;
    use = 0;
    association_tag = 0;
    selector_type.reset();
    transaction_id = 0;
    timeout = 0;
    selector_bytes.clear();
}


//----------------------------------------------------------------------------
// Serialize the Tap().
//----------------------------------------------------------------------------

void ts::DSMCCTap::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(id);
    buf.putUInt16(use);
    buf.putUInt16(association_tag);
    buf.pushWriteSequenceWithLeadingLength(8);
    if (selector_type.has_value()) {
        buf.putUInt16(selector_type.value());
        if (selector_type == DSMCC_TAPSELTYPE_MESSAGE) {
            buf.putUInt32(transaction_id);
            buf.putUInt32(timeout);
        }
        else {
            buf.putBytes(selector_bytes);
        }
    }
    buf.popState();
}


//----------------------------------------------------------------------------
// Deserialize the Tap().
//----------------------------------------------------------------------------

void ts::DSMCCTap::deserialize(PSIBuffer& buf)
{
    clear();
    id = buf.getUInt16();
    use = buf.getUInt16();
    association_tag = buf.getUInt16();
    buf.pushReadSizeFromLength(8);
    if (buf.canRead()) {
        selector_type = buf.getUInt16();
        if (selector_type == DSMCC_TAPSELTYPE_MESSAGE) {
            transaction_id = buf.getUInt32();
            timeout = buf.getUInt32();
        }
        else {
            buf.getBytes(selector_bytes);
        }
    }
    buf.popState();
}


//----------------------------------------------------------------------------
// A static method to display a Tap().
//----------------------------------------------------------------------------

bool ts::DSMCCTap::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(7)) {
        return false;
    }
    disp << margin << UString::Format(u"DSM-CC Tap: id: %n", buf.getUInt16()) << std::endl;
    disp << margin << "  Tap use: " << NameFromSection(u"dtv", u"DSMCC.tap_use", buf.getUInt16(), NamesFlags::HEX_VALUE_NAME) << std::endl;
    disp << margin << UString::Format(u"  Association tag: %n", buf.getUInt16()) << std::endl;
    buf.pushReadSizeFromLength(8);
    if (buf.canReadBytes(2)) {
        const uint16_t selector_type = buf.getUInt16();
        disp << margin << UString::Format(u"  Selector type: %n", selector_type) << std::endl;
        if (selector_type != DSMCC_TAPSELTYPE_MESSAGE) {
            disp.displayPrivateData(u"Selector bytes", buf, NPOS, margin + u"  ");
        }
        else if (buf.canReadBytes(8)) {
            disp << margin << UString::Format(u"  Transaction id: %n", buf.getUInt32());
            disp << UString::Format(u", timeout: %'d microseconds", buf.getUInt32()) << std::endl;
        }
    }
    disp.displayPrivateData(u"Extraneous data in selector", buf, NPOS, margin);
    buf.popState();
    return !buf.error();
}


//----------------------------------------------------------------------------
// This method converts a Tap() to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::DSMCCTap::toXML(DuckContext& duck, xml::Element* parent, const UChar* xml_name) const
{
    xml::Element* element = parent->addElement(xml_name);
    element->setIntAttribute(u"id", id, true);
    element->setIntAttribute(u"use", use, true);
    element->setIntAttribute(u"association_tag", association_tag, true);
    if (selector_type.has_value()) {
        element->setIntAttribute(u"selector_type", selector_type.value(), true);
        if (selector_type == DSMCC_TAPSELTYPE_MESSAGE) {
            element->setIntAttribute(u"transaction_id", transaction_id, true);
            element->setIntAttribute(u"timeout", timeout);
        }
        else {
            element->addHexaTextChild(u"selector_bytes", selector_bytes, true);
        }
    }
    return element;
}


//----------------------------------------------------------------------------
// This method decodes an XML Tap().
//----------------------------------------------------------------------------

bool ts::DSMCCTap::fromXML(DuckContext& duck, const xml::Element* parent, const UChar* xml_name)
{
    clear();

    // Get the Tap() element.
    const xml::Element* e = parent;
    if (xml_name != nullptr) {
        xml::ElementVector children;
        if (!parent->getChildren(children, xml_name, 1, 1)) {
            return false;
        }
        e = children[0];
    }

    // Analyze the Tap() element.
    bool ok = e->getIntAttribute(id, u"id", true) &&
              e->getIntAttribute(use, u"use", true) &&
              e->getIntAttribute(association_tag, u"association_tag", true) &&
              e->getOptionalIntAttribute(selector_type, u"selector_type");
    if (ok && selector_type.has_value()) {
        if (selector_type == DSMCC_TAPSELTYPE_MESSAGE) {
            ok = e->getIntAttribute(transaction_id, u"transaction_id", true) &&
                 e->getIntAttribute(timeout, u"timeout", true);
        }
        else {
            ok = e->getHexaTextChild(selector_bytes, u"selector_bytes", false, 0, 253);
        }
    }
    if (ok && selector_type != DSMCC_TAPSELTYPE_MESSAGE && (e->hasAttribute(u"transaction_id") || e->hasAttribute(u"timeout"))) {
        parent->report().error(u"line %d: in <%s>, attributes transaction_id and timeout allowed only when selector_type is %d", e->lineNumber(), e->name(), DSMCC_TAPSELTYPE_MESSAGE);
        ok = false;

    }
    if (ok && (!selector_type.has_value() || selector_type == DSMCC_TAPSELTYPE_MESSAGE)) {
        const xml::Element* sbytes = e->findFirstChild(u"selector_bytes", true);
        if (sbytes != nullptr) {
            parent->report().error(u"line %d: in <%s>, <selector_bytes> allowed only when selector_type is present and not %d", sbytes->lineNumber(), e->name(), DSMCC_TAPSELTYPE_MESSAGE);
            ok = false;
        }
    }
    return ok;
}

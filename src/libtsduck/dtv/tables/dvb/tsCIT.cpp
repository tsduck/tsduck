//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CIT"
#define MY_CLASS ts::CIT
#define MY_TID ts::TID_CIT
#define MY_PID ts::PID_CIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CIT::CIT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_)
{
}

ts::CIT::CIT(DuckContext& duck, const BinaryTable& table) :
    CIT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::CIT::tableIdExtension() const
{
    return service_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::CIT::clearContent()
{
    service_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    prepend_strings.clear();
    crids.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    service_id = section.tableIdExtension();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();

    // List of prepend strings, zero separated.
    UStringVector pstring;
    buf.getUTF8WithLength().split(pstring, CHAR_NULL, false, false);

    // In theory, each section may have a distinct list of prepend strings.
    // The prepend_string_index in a CRID is an index in the prepend strings of this section.
    // We group all prepend strings from all sections in one single vector and
    // we consequently need to translate the prepend string indexes of this section
    // into indexes in the global prepend_strings vector.
    std::vector<uint8_t> index_translation(pstring.size(), 0xFF);

    // Merge prepend strings of this section into global prepend_strings.
    for (size_t i1 = 0; i1 < pstring.size(); ++i1) {
        // Check if pstring[i1] is already in the global prepend_strings vector.
        for (size_t i2 = 0; i2 < prepend_strings.size(); ++i2) {
            if (prepend_strings[i2] == pstring[i1]) {
                index_translation[i1] = uint8_t(i2);
                break;
            }
        }
        if (index_translation[i1] == 0xFF) {
            // pstring[i1] not found in global prepend_strings, add it.
            index_translation[i1] = uint8_t(prepend_strings.size());
            prepend_strings.push_back(pstring[i1]);
        }
    }

    // Get list of CRID.
    while (buf.canRead()) {
        CRID cr;
        cr.crid_ref = buf.getUInt16();
        cr.prepend_string_index = buf.getUInt8();
        cr.prepend_string_index = cr.prepend_string_index < index_translation.size() ? index_translation[cr.prepend_string_index] : 0xFF;
        buf.getUTF8WithLength(cr.unique_string);
        crids.push_back(cr);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUTF8WithLength(UString::Join(prepend_strings, UString(1, CHAR_NULL)));
    buf.pushState();

    // Add all CRID's.
    bool retry = false;
    auto it = crids.begin();
    while (!buf.error() && it != crids.end()) {
        const CRID& cr(*it);

        // Try to serialize the current CRID in the current section.
        // Keep current position in case we cannot completely serialize it.
        buf.pushState();
        buf.putUInt16(cr.crid_ref);
        buf.putUInt8(cr.prepend_string_index);
        buf.putUTF8WithLength(cr.unique_string);

        // Handle end of serialization for current CRID.
        if (!buf.error()) {
            // CRID was successfully serialized. Move to next one.
            retry = false;
            buf.dropState(); // drop initially saved position.
            ++it;
        }
        else if (retry) {
            // This is already a retry on an empty section. Definitely too large, invalid table.
            return;
        }
        else {
            // Could not serialize in this section, try with an empty one.
            retry = true;
            buf.popState(); // return to previous state before current CRID.
            buf.clearError();
            addOneSection(table, buf);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a CIT section.
//----------------------------------------------------------------------------

void ts::CIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        UStringVector pstring;
        buf.getUTF8WithLength().split(pstring, CHAR_NULL, false, false);
        disp << margin << "Number of prepend string: " << pstring.size() << std::endl;
        for (size_t i = 0; i < pstring.size(); ++i) {
            disp << margin << "  Prepend[" << i << "] = \"" << pstring[i] << "\"" << std::endl;
        }
        while (buf.canReadBytes(5)) {
            disp << margin << UString::Format(u"- CRID reference: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Prepend string index: %d", {buf.getUInt8()}) << std::endl;
            disp << margin << "  Unique string: \"" << buf.getUTF8WithLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    for (const auto& it : prepend_strings) {
        root->addElement(u"prepend_string")->setAttribute(u"value", it);
    }
    for (const auto& it : crids) {
        xml::Element* e = root->addElement(u"crid");
        e->setIntAttribute(u"crid_ref", it.crid_ref, true);
        e->setIntAttribute(u"prepend_string_index", it.prepend_string_index, false);
        e->setAttribute(u"unique_string", it.unique_string);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xprepend;
    xml::ElementVector xcrid;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(service_id, u"service_id", true) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getChildren(xprepend, u"prepend_string", 0, 254) &&
        element->getChildren(xcrid, u"crid");

    for (auto it = xprepend.begin(); ok && it != xprepend.end(); ++it) {
        UString str;
        ok = (*it)->getAttribute(str, u"value", true);
        prepend_strings.push_back(str);
    }

    for (auto it = xcrid.begin(); ok && it != xcrid.end(); ++it) {
        CRID cr;
        ok = (*it)->getIntAttribute(cr.crid_ref, u"crid_ref", true) &&
             (*it)->getIntAttribute(cr.prepend_string_index, u"prepend_string_index", true) &&
             (*it)->getAttribute(cr.unique_string, u"unique_string", true, UString(), 0, 255);
        if (ok && cr.prepend_string_index >= prepend_strings.size() && cr.prepend_string_index != 0xFF) {
            element->report().error(u"line %d, attribute 'prepend_string_index' out of range", {(*it)->lineNumber()});
            ok = false;
        }
        crids.push_back(cr);
    }
    return ok;
}

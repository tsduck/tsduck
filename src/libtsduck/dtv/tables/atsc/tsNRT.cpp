//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNRT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NRT"
#define MY_CLASS ts::NRT
#define MY_TID ts::TID_NRT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NRT::NRT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, CURRENT)
{
}

ts::NRT::NRT(DuckContext& duck, const BinaryTable& table) :
    NRT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::NRT::isCurrent() const
{
    return CURRENT;
}

void ts::NRT::setCurrent(bool is_current)
{
    _is_current = CURRENT;
}

uint16_t ts::NRT::tableIdExtension() const
{
    return table_id_extension;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::NRT::clearContent()
{
    table_id_extension = 0xFFFF;
    resources.clear();
    private_data.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NRT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    table_id_extension = section.tableIdExtension();

    size_t resource_descriptor_count_in_section = buf.getUInt8();
    while (!buf.error() && resource_descriptor_count_in_section-- > 0) {
        Resource& res(resources.emplace_back());
        res.compatibility_descriptor.deserialize(buf);
        res.resource_descriptor.deserialize(buf);
    }
    buf.getBytes(private_data, buf.getUInt16());
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NRT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Save position before resource_descriptor_count_in_section. Will be updated at each resource.
    uint8_t resource_descriptor_count_in_section = 0;
    buf.pushState();
    buf.putUInt8(resource_descriptor_count_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset() + 2;

    // Loop on resource descriptions.
    for (auto res = resources.begin(); !buf.error() && res != resources.end(); ++res) {

        // We don't know the total size of the serialized resource description and we don't know if it will fit
        // in the current section. So, we serialize the complete resource into one specific buffer first.
        // Then, we will know if we can copy it in the current section or if we must create a new one.
        PSIBuffer resbuf(buf.duck(), MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE - payload_min_size);

        // Serialize the complete resource description in resbuf.
        res->compatibility_descriptor.serialize(buf);
        res->resource_descriptor.serialize(buf);
        const size_t res_size = resbuf.currentWriteByteOffset();

        // If we are not at the beginning of the resource loop, make sure that the entire
        // resource fits in the section. If it does not fit, start a new section.
        if (res_size + 2 > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Finish the section.
            buf.putUInt16(0); // private_data_length
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of resource_descriptor_count_in_section in the new section.
            buf.putUInt8(resource_descriptor_count_in_section = 0);
        }

        // Copy the serialized resource definition.
        buf.putBytes(resbuf.currentReadAddress(), res_size);

        // Now increment the field application_count_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++resource_descriptor_count_in_section);
        buf.popState();
        buf.swapState();
    }

    // Finally serialize service_private_data.
    if (2 + private_data.size() > buf.remainingWriteBytes()) {
        // Complete the section and creates a new one.
        buf.putUInt16(0); // private_data_length
        addOneSection(table, buf);
        // We are at the position of resource_descriptor_count_in_section in the new section.
        buf.putUInt8(0);  // resource_descriptor_count_in_section
    }
    buf.putUInt16(uint16_t(private_data.size()));
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// A static method to display a NRT section.
//----------------------------------------------------------------------------

void ts::NRT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Table id extension: %n", section.tableIdExtension()) << std::endl;

    // Payload initial fixed part.
    if (!buf.canReadBytes(1)) {
        return;
    }
    const size_t res_count = buf.getUInt8();
    disp << margin << "Number of resources: " << res_count << std::endl;

    // Loop on all resources.
    for (size_t res_index = 0; res_index < res_count; ++res_index) {
        disp << margin << "- Resource #" << res_index << std::endl;
        if (!DSMCCCompatibilityDescriptor::Display(disp, buf, margin + u"  ") ||
            !DSMCCResourceDescriptor::Display(disp, buf, margin + u"  "))
        {
            return;
        }
    }

    // Private data.
    if (buf.canReadBytes(2)) {
        disp.displayPrivateData(u"Private data", buf, buf.getUInt16(), margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NRT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"table_id_extension", table_id_extension, true);

    for (const auto& res : resources) {
        xml::Element* e = root->addElement(u"resource");
        res.compatibility_descriptor.toXML(duck, e);
        res.resource_descriptor.toXML(duck, e);
    }

    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NRT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xres;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getIntAttribute(table_id_extension, u"table_id_extension", false, 0xFFFF) &&
              element->getChildren(xres, u"resource") &&
              element->getHexaTextChild(private_data, u"private_data");

    for (auto xr : xres) {
        Resource& res(resources.emplace_back());
        ok = res.compatibility_descriptor.fromXML(duck, xr) &&
             res.resource_descriptor.fromXML(duck, xr) &&
             ok;
    }
    return ok;
}

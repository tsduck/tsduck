//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsContainerTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsZlib.h"

#define MY_XML_NAME u"container_table"
#define MY_CLASS ts::ContainerTable
#define MY_TID ts::TID_CT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContainerTable::ContainerTable(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_)
{
}

ts::ContainerTable::ContainerTable(DuckContext& duck, const BinaryTable& table) :
    ContainerTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ContainerTable::tableIdExtension() const
{
    return container_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ContainerTable::clearContent()
{
    container_id = 0;
    compression_wrapper.clear();
}


//----------------------------------------------------------------------------
// Extract the container binary data block from the compression wrapper.
//----------------------------------------------------------------------------

bool ts::ContainerTable::getContainer(ByteBlock& container) const
{
    container.clear();

    // See ETSI TS 102 323, 7.3.1.5.
    if (compression_wrapper.empty()) {
        return false;
    }
    else if (compression_wrapper[0] == 0) {
        // Uncompressed
        container.copy(compression_wrapper.data() + 1, compression_wrapper.size() - 1);
        return true;
    }
    else if (compression_wrapper[0] == 1 && compression_wrapper.size() >= 4) {
        // Zlib-compressed.
        const size_t original_size = GetUInt24(compression_wrapper.data() + 1);
        return Zlib::Decompress(container, compression_wrapper.data() + 4, compression_wrapper.size() - 4) && container.size() == original_size;
    }
    else {
        // Invalid compression method.
        return false;
    }
}


//----------------------------------------------------------------------------
// Store the container binary data block into the compression wrapper.
//----------------------------------------------------------------------------

bool ts::ContainerTable::setContainer(const ByteBlock& container, bool compress)
{
    static constexpr int default_compression_level = 6; // range is 0-9
    compression_wrapper.clear();
    if (compress) {
        compression_wrapper.push_back(1);
        compression_wrapper.appendUInt24(uint32_t(container.size()));
        return Zlib::CompressAppend(compression_wrapper, container, default_compression_level);
    }
    else {
        compression_wrapper.push_back(0);
        compression_wrapper.append(container);
        return true;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContainerTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    container_id = section.tableIdExtension();
    buf.getBytesAppend(compression_wrapper);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContainerTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    const uint8_t* data = compression_wrapper.data();
    size_t remain = compression_wrapper.size();

    // Build at least one section.
    do {
        const size_t size = std::min(remain, MAX_CONTAINER_DATA);
        buf.putBytes(data, size);
        data += size;
        remain -= size;
        if (remain > 0) {
            addOneSection(table, buf);
        }
    } while (remain > 0);
}


//----------------------------------------------------------------------------
// A static method to display a ContainerTable section.
//----------------------------------------------------------------------------

void ts::ContainerTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Container id: %n", section.tableIdExtension()) << std::endl;
    disp.displayPrivateData(u"Container data", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContainerTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"container_id", container_id, true);
    root->addHexaTextChild(u"compression_wrapper", compression_wrapper, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContainerTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(_is_current, u"current", false, true) &&
           element->getIntAttribute(container_id, u"container_id", true) &&
           element->getHexaTextChild(compression_wrapper, u"compression_wrapper");
}

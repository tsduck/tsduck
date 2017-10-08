//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  XML files containing PSI/SI tables.
//
//----------------------------------------------------------------------------

#include "tsXMLTables.h"
#include "tsAbstractTable.h"
#include "tsAbstractDescriptor.h"
#include "tsBinaryTable.h"
#include "tsTablesFactory.h"
#include "tsStringUtils.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

#define XML_GENERIC_DESCRIPTOR   "generic_descriptor"
#define XML_GENERIC_SHORT_TABLE  "generic_short_table"
#define XML_GENERIC_LONG_TABLE   "generic_long_table"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::XMLTables::XMLTables() :
    _tables()
{
}


//----------------------------------------------------------------------------
// Add a table in the file.
//----------------------------------------------------------------------------

void ts::XMLTables::add(const AbstractTablePtr& table)
{
    if (!table.isNull() && table->isValid()) {
        BinaryTablePtr bin(new BinaryTable);
        table->serialize(*bin);
        if (bin->isValid()) {
            add(bin);
        }
    }
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::XMLTables::loadXML(const std::string& file_name, ReportInterface& report)
{
    clear();
    XML xml(report);
    XML::Document doc;
    return xml.loadDocument(doc, file_name, false) && parseDocument(xml, doc);
}

bool ts::XMLTables::parseXML(const std::string& xml_content, ReportInterface& report)
{
    clear();
    XML xml(report);
    XML::Document doc;
    return xml.parseDocument(doc, xml_content) && parseDocument(xml, doc);
}

bool ts::XMLTables::parseDocument(XML& xml, const XML::Document& doc)
{
    // Load the XML model for TSDuck files. Search it in TSDuck directory.
    XML::Document model;
    if (!xml.loadDocument(model, "tsduck.xml", true)) {
        xml.reportError("Model for TSDuck XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!xml.validateDocument(model, doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const XML::Element* root = doc.RootElement();
    bool success = true;

    // Analyze all tables in the document.
    for (const XML::Element* node = root == 0 ? 0 : root->FirstChildElement(); node != 0; node = node->NextSiblingElement()) {

        BinaryTablePtr bin;
        const std::string name(XML::ElementName(node));

        // Get the table factory for that kind of XML tag.
        const TablesFactory::TableFactory fac = TablesFactory::Instance()->getTableFactory(name);
        if (fac != 0) {
            // Create a table instance of the right type.
            AbstractTablePtr table = fac();
            if (!table.isNull()) {
                table->fromXML(xml, node);
            }
            if (!table.isNull() && table->isValid()) {
                // Serialize the table.
                bin = new BinaryTable;
                table->serialize(*bin);
            }
        }
        else {
            // No known factory, add a generic table.
            bin = FromGenericTableXML(xml, node);
        }

        // Insert created table or report error.
        if (!bin.isNull() && bin->isValid()) {
            _tables.push_back(bin);
        }
        else {
            xml.reportError(Format("Error in table <%s> at line %d", name.c_str(), node->GetLineNum()));
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::XMLTables::saveXML(const std::string& file_name, ReportInterface& report) const
{
    // Create the file.
    ::FILE* fp = ::fopen(file_name.c_str(), "w");
    if (fp == 0) {
        report.error("cannot create file " + file_name);
        return false;
    }

    // Generate the XML content.
    XML xml(report);
    XML::Printer printer(2, fp);
    const bool success = generateDocument(xml, printer);

    // Cleanup.
    ::fclose(fp);
    return success;
}

std::string ts::XMLTables::toText(ReportInterface& report) const
{
    // Generate the XML content.
    XML xml(report);
    XML::Printer printer(2);
    if (!generateDocument(xml, printer)) {
        return std::string();
    }

    // Get result and cleanup end of lines.
    std::string text(printer.CStr());
    SubstituteAll(text, "\r", "");
    return text;
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::XMLTables::generateDocument(XML& xml, XML::Printer& printer) const
{
    // Initialize the document structure.
    XML::Document doc;
    XML::Element* root = xml.initializeDocument(&doc, "tsduck");
    if (root == 0) {
        return false;
    }

    // Format all tables.
    for (BinaryTablePtrVector::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
        const BinaryTablePtr& table(*it);
        if (!table.isNull()) {
            ToXML(xml, root, *table);
        }
    }

    // Format the document.
    doc.Print(&printer);
    return true;
}


//----------------------------------------------------------------------------
// This method converts a table to the appropriate XML tree.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XMLTables::ToXML(XML& xml, XML::Element* parent, const BinaryTable& table, const DVBCharset* charset)
{
    // Filter invalid tables.
    if (!table.isValid()) {
        return 0;
    }

    // The XML node we will generate.
    XML::Element* node = 0;

    // Do we know how to deserialize this table?
    TablesFactory::TableFactory fac = TablesFactory::Instance()->getTableFactory(table.tableId());
    if (fac != 0) {
        // We know how to deserialize this table.
        AbstractTablePtr tp = fac();
        if (!tp.isNull()) {
            // Deserialize from binary to object.
            tp->deserialize(table, charset);
            if (tp->isValid()) {
                // Serialize from object to XML.
                node = tp->toXML(xml, parent);
            }
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == 0) {
        node = ToGenericTable(xml, parent, table);
    }

    return node;
}


//----------------------------------------------------------------------------
// This method converts a descriptor to the appropriate XML tree.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XMLTables::ToXML(XML& xml, XML::Element* parent, const Descriptor& desc, PDS pds, const DVBCharset* charset)
{
    // Filter invalid descriptors.
    if (!desc.isValid()) {
        return 0;
    }

    // The XML node we will generate.
    XML::Element* node = 0;

    // Do we know how to deserialize this descriptor?
    TablesFactory::DescriptorFactory fac = TablesFactory::Instance()->getDescriptorFactory(desc.edid(pds));
    if (fac != 0) {
        // We know how to deserialize it.
        AbstractDescriptorPtr dp = fac();
        if (!dp.isNull()) {
            // Deserialize from binary to object.
            dp->deserialize(desc, charset);
            if (dp->isValid()) {
                // Serialize from object to XML.
                node = dp->toXML(xml, parent);
            }
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == 0) {
        node = ToGenericDescriptor(xml, parent, desc);
    }

    return node;
}


//----------------------------------------------------------------------------
// This method converts a list of descriptors to XML.
//----------------------------------------------------------------------------

bool ts::XMLTables::ToXML(XML& xml, XML::Element* parent, const DescriptorList& list, const DVBCharset* charset)
{
    bool success = true;
    for (size_t index = 0; index < list.count(); ++index) {
        const DescriptorPtr desc(list[index]);
        if (desc.isNull() || ToXML(xml, parent, *desc, list.privateDataSpecifier(index), charset) == 0) {
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// This method converts a generic table to XML.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XMLTables::ToGenericTable(XML& xml, XML::Element* parent, const BinaryTable& table)
{
    // Filter invalid tables.
    if (!table.isValid() || table.sectionCount() == 0) {
        return 0;
    }
    SectionPtr section(table.sectionAt(0));
    if (section.isNull()) {
        return 0;
    }

    if (table.isShortSection()) {
        // Create a short section node.
        XML::Element* root = xml.addElement(parent, "generic_short_table");
        xml.setIntAttribute(root, "table_id", section->tableId(), true);
        xml.setBoolAttribute(root, "private", section->isPrivateSection());
        xml.addHexaText(root, section->payload(), section->payloadSize());
        return root;
    }
    else {
        // Create a table with long sections.
        XML::Element* root = xml.addElement(parent, "generic_long_table");
        xml.setIntAttribute(root, "table_id", table.tableId(), true);
        xml.setIntAttribute(root, "table_id_ext", table.tableIdExtension(), true);
        xml.setIntAttribute(root, "version", table.version());
        xml.setBoolAttribute(root, "current", section->isCurrent());
        xml.setBoolAttribute(root, "private", section->isPrivateSection());

        // Add each section in binary format.
        for (size_t index = 0; index < table.sectionCount(); ++index) {
            section = table.sectionAt(index);
            if (!section.isNull() && section->isValid()) {
                XML::Element* child = xml.addElement(root, "section");
                xml.addHexaText(child, section->payload(), section->payloadSize());
            }
        }
        return root;
    }
}


//----------------------------------------------------------------------------
// This method converts a generic descriptor to XML.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XMLTables::ToGenericDescriptor(XML& xml, XML::Element* parent, const Descriptor& desc)
{
    // Filter invalid descriptor.
    if (!desc.isValid()) {
        return 0;
    }

    // Create the XML node.
    XML::Element* root = xml.addElement(parent, XML_GENERIC_DESCRIPTOR);
    xml.setIntAttribute(root, "tag", desc.tag(), true);
    xml.addHexaText(root, desc.payload(), desc.payloadSize());
    return root;
}


//----------------------------------------------------------------------------
// This method decodes an XML list of descriptors.
//----------------------------------------------------------------------------

bool ts::XMLTables::FromDescriptorListXML(DescriptorList& list, XML::ElementVector& others, XML& xml, const XML::Element* parent, const StringList& allowedOthers, const DVBCharset* charset)
{
    bool success = true;
    list.clear();
    others.clear();

    // Analyze all children nodes.
    for (const XML::Element* node = parent == 0 ? 0 : parent->FirstChildElement(); node != 0; node = node->NextSiblingElement()) {

        DescriptorPtr bin;
        const std::string name(XML::ElementName(node));
        bool isDescriptor = false;

        // Get the descriptor factory for that kind of XML tag.
        const TablesFactory::DescriptorFactory fac = TablesFactory::Instance()->getDescriptorFactory(name);
        if (fac != 0) {
            isDescriptor = true;
            // Create a descriptor instance of the right type.
            AbstractDescriptorPtr desc = fac();
            if (!desc.isNull()) {
                desc->fromXML(xml, node);
            }
            if (!desc.isNull() && desc->isValid()) {
                // Serialize the descriptor.
                bin = new Descriptor;
                desc->serialize(*bin, charset);
            }
        }
        else if (SimilarStrings(name, XML_GENERIC_DESCRIPTOR)) {
            isDescriptor = true;
            // Add a generic descriptor.
            bin = FromGenericDescriptorXML(xml, node);
        }

        if (isDescriptor) {
            // The tag is a valid descriptor name.
            if (!bin.isNull() && bin->isValid()) {
                list.add(bin);
            }
            else {
                xml.reportError(Format("Error in descriptor <%s> at line %d", name.c_str(), node->GetLineNum()));
                success = false;
            }
        }
        else {
            // The tag is not a descriptor name, check if this is one of the allowed node.
            if (ContainSimilarString(allowedOthers, name)) {
                others.push_back(node);
            }
            else {
                xml.reportError(Format("Illegal <%s> at line %d", name.c_str(), node->GetLineNum()));
                success = false;
            }
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// This method decodes a <generic_short_table> or <generic_long_table>.
//----------------------------------------------------------------------------

ts::BinaryTablePtr ts::XMLTables::FromGenericTableXML(XML& xml, const XML::Element* elem)
{
    // Silently ignore invalid parameters.
    if (elem == 0) {
        return BinaryTablePtr();
    }

    // There are two possible forms of generic tables.
    const char* const name = XML::ElementName(elem);
    if (SimilarStrings(name, XML_GENERIC_SHORT_TABLE)) {

        TID tid = 0xFF;
        bool priv = true;
        ByteBlock payload;
        bool ok =
            xml.getIntAttribute<TID>(tid, elem, "table_id", true, 0xFF, 0x00, 0xFF) &&
            xml.getBoolAttribute(priv, elem, "private", false, true) &&
            xml.getHexaText(payload, elem, 0, MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE);

        if (ok) {
            BinaryTablePtr table(new BinaryTable);
            table->addSection(SectionPtr(new Section(tid, priv, payload.data(), payload.size())));
            if (table->isValid()) {
                return table;
            }
        }
    }
    else if (SimilarStrings(name, XML_GENERIC_LONG_TABLE)) {

        TID tid = 0xFF;
        uint16_t tidExt = 0xFFFF;
        uint8_t version = 0;
        bool priv = true;
        bool current = true;
        XML::ElementVector sectionNodes;
        bool ok =
            xml.getIntAttribute<TID>(tid, elem, "table_id", true, 0xFF, 0x00, 0xFF) &&
            xml.getIntAttribute<uint16_t>(tidExt, elem, "table_id_ext", false, 0xFFFF, 0x0000, 0xFFFF) &&
            xml.getIntAttribute<uint8_t>(version, elem, "version", false, 0, 0, 31) &&
            xml.getBoolAttribute(current, elem, "current", false, true) &&
            xml.getBoolAttribute(priv, elem, "private", false, true) &&
            xml.getChildren(sectionNodes, elem, "section", 1, 256);

        if (ok) {
            BinaryTablePtr table(new BinaryTable);
            for (size_t index = 0; ok && index < sectionNodes.size(); ++index) {
                assert(sectionNodes[index] != 0);
                ByteBlock payload;
                ok = xml.getHexaText(payload, sectionNodes[index], 0, MAX_PSI_LONG_SECTION_PAYLOAD_SIZE);
                if (ok) {
                    table->addSection(SectionPtr(new Section(tid, priv, tidExt, version, current, uint8_t(index), uint8_t(index), payload.data(), payload.size())));
                }
            }
            if (ok && table->isValid()) {
                return table;
            }
        }
    }

    // At this point, the table is invalid.
    xml.reportError(Format("<%s>, line %d, is not a valid table", name, elem->GetLineNum()));
    return BinaryTablePtr();
}


//----------------------------------------------------------------------------
// This method decodes a <generic_descriptor>.
//----------------------------------------------------------------------------

ts::DescriptorPtr ts::XMLTables::FromGenericDescriptorXML(XML& xml, const XML::Element* elem)
{
    // Silently ignore invalid parameters.
    if (elem == 0) {
        return DescriptorPtr();
    }

    // Decode XML.
    DID tag = 0xFF;
    ByteBlock payload;
    const bool ok =
        SimilarStrings(XML::ElementName(elem), XML_GENERIC_DESCRIPTOR) &&
        xml.getIntAttribute<DID>(tag, elem, "tag", true, 0xFF, 0x00, 0xFF) &&
        xml.getHexaText(payload, elem, 0, 255);

    // Build descriptor.
    if (ok) {
        return DescriptorPtr(new Descriptor(tag, payload));
    }
    else {
        xml.reportError(Format("<%s>, line %d, is not a valid descriptor", XML::ElementName(elem), elem->GetLineNum()));
        return DescriptorPtr();
    }
}

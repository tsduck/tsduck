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

bool ts::XMLTables::load(const std::string& file_name, ReportInterface& report)
{
    clear();
    XML xml(report);
    XML::Document doc;
    return xml.loadDocument(doc, file_name, false) && parseDocument(xml, doc);
}

bool ts::XMLTables::parse(const std::string& xml_content, ReportInterface& report)
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
        const char* name = node->Name();
        if (name != 0) {

            // Get the table factory for that kind of XML tag.
            /*@@@@@@@@@@@@@@@@@@@@@@@@@
            TableFactories::const_iterator it = TableFactories::Instance()->find(std::string(name));

            if (it == TableFactories::Instance()->end()) {
                xml.reportError(Format("Unknown table type <%s> at line %d", name, node->GetLineNum()));
                success = false;
            }
            else {
                // Create a table instance of the right type.
                AbstractTablePtr table = (it->second)();
                if (!table.isNull()) {
                    table->fromXML(xml, node);
                }
                if (table.isNull() || !table->isValid()) {
                    xml.reportError(Format("Error in table <%s> at line %d", name, node->GetLineNum()));
                    success = false;
                }
            }

            @@@@@@@@@@@@@@*/
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::XMLTables::save(const std::string& file_name, ReportInterface& report) const
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
        if (!table.isNull() && table->isValid()) {

            // The XML node we will generate.
            XML::Element* node = 0;

            // Do we know how to deserialize this table?
            TablesFactory::TableFactory fac = TablesFactory::Instance()->getTableFactory(table->tableId());
            if (fac != 0) {
                // We know how to deserialize this table.
                AbstractTablePtr tp = fac();
                if (!tp.isNull()) {
                    // Deserialize from binary to object.
                    tp->deserialize(*table);
                    if (tp->isValid()) {
                        // Serialize from object to XML.
                        node = tp->toXML(xml, doc);
                    }
                }
            }

            // If we could not generate a typed node, generate a generic one.
            if (node == 0) {
                node = ToGenericTable(xml, doc, *table);
            }

            // Finaly add the XML node inside the document.
            if (node != 0) {
                root->InsertEndChild(node);
            }
        }
    }

    // Format the document.
    doc.Print(&printer);
    return true;
}


//----------------------------------------------------------------------------
// This method converts a generic table to XML.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XMLTables::ToGenericTable(XML& xml, XML::Document& doc, const BinaryTable& table)
{
    // Filter invalid tables.
    if (!table.isValid()) {
        return 0;
    }

    if (table.isShortSection()) {
        // Create a short section node.
        SectionPtr section(table.sectionAt(0));
        if (section.isNull()) {
            return 0;
        }
        XML::Element* root = doc.NewElement("generic_short_table");
        if (root == 0) {
            return 0;
        }
        root->SetAttribute("table_id", int(table.tableId()));
        root->SetAttribute("private", int(section->isPrivateSection()));
        root->SetAttribute("reserved1", int((section->content()[1] >> 4) & 0x03));
        xml.addHexaText(root, section->payload(), section->payloadSize());
        return root;
    }
    else {
        // Create a long section.
        XML::Element* root = doc.NewElement("generic_long_table");
        if (root == 0) {
            return 0;
        }

        // Collect table information from the first section.
        if (table.sectionCount() == 0) {
            return 0;
        }
        SectionPtr section(table.sectionAt(0));
        if (section.isNull()) {
            return 0;
        }
        root->SetAttribute("table_id", int(table.tableId()));
        root->SetAttribute("table_id_ext", int(table.tableIdExtension()));
        root->SetAttribute("version", int(table.version()));
        root->SetAttribute("current", section->isCurrent());
        root->SetAttribute("private", int(section->isPrivateSection()));
        root->SetAttribute("reserved1", int((section->content()[1] >> 4) & 0x03));
        root->SetAttribute("reserved2", int((section->content()[5] >> 6) & 0x03));

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

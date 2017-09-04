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
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Generate XML file or content.
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

bool ts::XMLTables::generateDocument(XML& xml, XML::Printer& printer) const
{
    // Initialize the document structure.
    XML::Document doc;
    XML::Element* root = xml.initializeDocument(&doc, "tsduck");
    if (root == 0) {
        return false;
    }

    // Format all tables.
    for (AbstractTablePtrVector::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
        const AbstractTablePtr& table(*it);
        if (!table.isNull() && table->isValid()) {
            XML::Element* node = table->toXML(xml, doc);
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
// Table factories.
//----------------------------------------------------------------------------

tsDefineSingleton(ts::XMLTables::TableFactories);

ts::XMLTables::TableFactories::TableFactories() {}

ts::XMLTables::RegisterTableType::RegisterTableType(const std::string& node_name, TableFactory factory)
{
    TableFactories::Instance()->insert(std::pair<std::string,TableFactory>(node_name, factory));
}

void ts::XMLTables::GetRegisteredTableNames(StringList& names)
{
    names.clear();
    for (std::map<std::string,TableFactory>::const_iterator it = TableFactories::Instance()->begin(); it != TableFactories::Instance()->end(); ++it) {
        names.push_back(it->first);
    }
}


//----------------------------------------------------------------------------
// Descriptor factories.
//----------------------------------------------------------------------------

tsDefineSingleton(ts::XMLTables::DescriptorFactories);

ts::XMLTables::DescriptorFactories::DescriptorFactories() {}

ts::XMLTables::RegisterDescriptorType::RegisterDescriptorType(const std::string& node_name, DescriptorFactory factory)
{
    DescriptorFactories::Instance()->insert(std::pair<std::string,DescriptorFactory>(node_name, factory));
}

void ts::XMLTables::GetRegisteredDescriptorNames(StringList& names)
{
    names.clear();
    for (std::map<std::string,DescriptorFactory>::const_iterator it = DescriptorFactories::Instance()->begin(); it != DescriptorFactories::Instance()->end(); ++it) {
        names.push_back(it->first);
    }
}

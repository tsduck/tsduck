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

#include "tsSectionFile.h"
#include "tsAbstractTable.h"
#include "tsAbstractDescriptor.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SectionFile::SectionFile() :
    _tables(),
    _sections(),
    _orphanSections()
{
}

ts::SectionFile::~SectionFile()
{
}


//----------------------------------------------------------------------------
// Clear the list of loaded tables and sections.
//----------------------------------------------------------------------------

void ts::SectionFile::clear()
{
    _tables.clear();
    _sections.clear();
    _orphanSections.clear();
}


//----------------------------------------------------------------------------
// Add a table in the file.
//----------------------------------------------------------------------------

void ts::SectionFile::add(const AbstractTablePtr& table, const DVBCharset* charset)
{
    if (!table.isNull() && table->isValid()) {
        BinaryTablePtr bin(new BinaryTable);
        table->serialize(*bin, charset);
        if (bin->isValid()) {
            add(bin);
        }
    }
}

void ts::SectionFile::add(const BinaryTablePtrVector& tables)
{
    for (BinaryTablePtrVector::const_iterator it = tables.begin(); it != tables.end(); ++it) {
        add(*it);
    }
}

void ts::SectionFile::add(const BinaryTablePtr& table)
{
    if (!table.isNull() && table->isValid()) {
        // Add the table as a whole.
        _tables.push_back(table);
        // Add all its sections (none of them is orphan).
        for (size_t i = 0; i < table->sectionCount(); ++i) {
            _sections.push_back(table->sectionAt(i));
        }
    }
}


//----------------------------------------------------------------------------
// Add a section in the file.
//----------------------------------------------------------------------------

//@@@@


//----------------------------------------------------------------------------
// Check it a table can be formed using the last sections in _sections.
// If the sections were present at end of _orphanSections, they are removed.
//----------------------------------------------------------------------------

void ts::SectionFile::collectLastTable()
{
    //@@@@@
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadXML(const UString& file_name, Report& report, const DVBCharset* charset)
{
    clear();
    xml::Document doc(report);
    return doc.load(file_name, false) && parseDocument(doc, charset);
}

bool ts::SectionFile::parseXML(const UString& xml_content, Report& report, const DVBCharset* charset)
{
    clear();
    xml::Document doc(report);
    return doc.parse(xml_content) && parseDocument(doc, charset);
}

bool ts::SectionFile::parseDocument(const xml::Document& doc, const DVBCharset* charset)
{
    // Load the XML model for TSDuck files. Search it in TSDuck directory.
    xml::Document model(doc.report());
    if (!model.load(u"tsduck.xml", true)) {
        doc.report().error(u"Model for TSDuck XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!doc.validate(model)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    bool success = true;

    // Analyze all tables in the document.
    for (const xml::Element* node = root == 0 ? 0 : root->firstChildElement(); node != 0; node = node->nextSiblingElement()) {
        BinaryTablePtr bin(new BinaryTable);
        CheckNonNull(bin.pointer());
        if (bin->fromXML(node) && bin->isValid()) {
            _tables.push_back(bin);
        }
        else {
            doc.report().error(u"Error in table <%s> at line %d", {node->name(), node->lineNumber()});
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::SectionFile::saveXML(const UString& file_name, Report& report, const DVBCharset* charset) const
{
    xml::Document doc(report);
    return generateDocument(doc, charset) && doc.save(file_name);
}

ts::UString ts::SectionFile::toText(Report& report, const DVBCharset* charset) const
{
    xml::Document doc(report);
    return generateDocument(doc, charset) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::SectionFile::generateDocument(xml::Document& doc, const DVBCharset* charset) const
{
    // Initialize the document structure.
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == 0) {
        return false;
    }

    // Format all tables.
    for (BinaryTablePtrVector::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
        const BinaryTablePtr& table(*it);
        if (!table.isNull()) {
            table->toXML(root, false, charset);
        }
    }

    return true;
}

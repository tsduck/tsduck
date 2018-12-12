//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SectionFile::SectionFile() :
    _tables(),
    _sections(),
    _orphanSections(),
    _xmlTweaks(),
    _charset(),
    _crc_op(CRC32::IGNORE)
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

void ts::SectionFile::add(const AbstractTablePtr& table)
{
    if (!table.isNull() && table->isValid()) {
        BinaryTablePtr bin(new BinaryTable);
        table->serialize(*bin, _charset);
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

void ts::SectionFile::add(const SectionPtrVector& sections)
{
    for (SectionPtrVector::const_iterator it = sections.begin(); it != sections.end(); ++it) {
        add(*it);
    }
}

void ts::SectionFile::add(const SectionPtr& section)
{
    if (!section.isNull() && section->isValid()) {
        // Make the section part of the global list of sections.
        _sections.push_back(section);
        // Temporary push this section in the orphan list.
        _orphanSections.push_back(section);
        // Try to build a table from the list of orphans.
        collectLastTable();
    }
}


//----------------------------------------------------------------------------
// Check it a table can be formed using the last sections in _orphanSections.
//----------------------------------------------------------------------------

void ts::SectionFile::collectLastTable()
{
    // If there is no orphan section, nothing to do.
    if (_orphanSections.empty()) {
        return;
    }

    // Get a iterator to last section.
    SectionPtrVector::iterator first = _orphanSections.end();
    --first;
    assert(!first->isNull());
    assert((*first)->isValid());

    // A short section should be a table in itself, no need to dive further.
    // Long sections must be all present for the same table.
    if ((*first)->isLongSection()) {

        // Last section of the table.
        const SectionPtr last(*first);

        // Check if all sections are present in order.
        for (uint8_t num = last->lastSectionNumber(); ; --num) {
            assert(!first->isNull());
            assert((*first)->isValid());

            // Give up if the section is not the expected one for the table.
            if ((*first)->tableId() != last->tableId() ||
                (*first)->tableIdExtension() != last->tableIdExtension() ||
                (*first)->version() != last->version() ||
                (*first)->sectionNumber() != num ||
                (*first)->lastSectionNumber() != last->lastSectionNumber())
            {
                return;
            }

            // Reached the first section in the table?
            if (num == 0) {
                break;
            }

            // Move to previous section.
            if (first == _orphanSections.begin()) {
                return; // beginning of the table is missing.
            }
            else {
                --first;
            }
        }
    }

    // We have now identified sections for a complete table.
    BinaryTablePtr table(new BinaryTable);
    CheckNonNull(table.pointer());
    if (!table->addSections(first, _orphanSections.end(), false, false) || !table->isValid()) {
        // Invalid table after all.
        return;
    }

    // Built a valid table.
    _tables.push_back(table);
    _orphanSections.erase(first, _orphanSections.end());
}


//----------------------------------------------------------------------------
// Load a binary section file.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadBinary(const UString& file_name, Report& report)
{
    // Open the input file.
    std::ifstream strm(file_name.toUTF8().c_str(), std::ios::in | std::ios::binary);
    if (!strm.is_open()) {
        clear();
        report.error(u"cannot open %s", {file_name});
        return false;
    }

    // Load the section file.
    ReportWithPrefix report_internal(report, file_name + u": ");
    const bool success = loadBinary(strm, report_internal);
    strm.close();

    return success;
}

bool ts::SectionFile::loadBinary(std::istream& strm, Report& report)
{
    clear();

    // Read all binary sections one by one.
    for (;;) {
        SectionPtr sp(new Section);
        if (sp->read(strm, _crc_op, report)) {
            add(sp);
        }
        else {
            break;
        }
    }

    // Success if reached EOF without error.
    return strm.eof();
}


//----------------------------------------------------------------------------
// Save a binary section file.
//----------------------------------------------------------------------------

bool ts::SectionFile::saveBinary(const UString& file_name, Report& report) const
{
    // Create the output file.
    std::ofstream strm(file_name.toUTF8().c_str(), std::ios::out | std::ios::binary);
    if (!strm.is_open()) {
        report.error(u"error creating %s", {file_name});
        return false;
    }

    // Save sections.
    ReportWithPrefix report_internal(report, file_name + u": ");
    const bool success = saveBinary(strm, report_internal);
    strm.close();

    return success;
}

bool ts::SectionFile::saveBinary(std::ostream& strm, Report& report) const
{
    for (size_t i = 0; i < _sections.size() && strm.good(); ++i) {
        if (!_sections[i].isNull() && _sections[i]->isValid()) {
            _sections[i]->write(strm, report);
        }
    }
    return strm.good();
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadXML(const UString& file_name, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(file_name, false) && parseDocument(doc);
}

bool ts::SectionFile::loadXML(std::istream& strm, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(strm) && parseDocument(doc);
}

bool ts::SectionFile::parseXML(const UString& xml_content, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.parse(xml_content) && parseDocument(doc);
}

bool ts::SectionFile::parseDocument(const xml::Document& doc)
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
    for (const xml::Element* node = root == nullptr ? nullptr : root->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {
        BinaryTablePtr bin(new BinaryTable);
        CheckNonNull(bin.pointer());
        if (bin->fromXML(node) && bin->isValid()) {
            add(bin);
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

bool ts::SectionFile::saveXML(const UString& file_name, Report& report) const
{
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) && doc.save(file_name);
}

ts::UString ts::SectionFile::toXML(Report& report) const
{
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::SectionFile::generateDocument(xml::Document& doc) const
{
    // Initialize the document structure.
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == nullptr) {
        return false;
    }

    // Format all tables.
    for (BinaryTablePtrVector::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
        const BinaryTablePtr& table(*it);
        if (!table.isNull()) {
            table->toXML(root, false, _charset);
        }
    }

    // Issue a warning if incomplete tables were not saved.
    if (!_orphanSections.empty()) {
        doc.report().warning(u"%d orphan sections not saved in XML document (%d tables saved)", {_orphanSections.size(), _tables.size()});
    }

    return true;
}


//----------------------------------------------------------------------------
// Get a file type, based on a file name.
//----------------------------------------------------------------------------

ts::SectionFile::FileType ts::SectionFile::GetFileType(const UString& file_name, FileType type)
{
    if (type != UNSPECIFIED) {
        return type; // already known
    }
    const UString ext(PathSuffix(file_name).toLower());
    if (ext == TS_DEFAULT_XML_SECTION_FILE_SUFFIX) {
        return XML;
    }
    else if (ext == TS_DEFAULT_BINARY_SECTION_FILE_SUFFIX) {
        return BINARY;
    }
    else {
        return UNSPECIFIED;
    }
}


//----------------------------------------------------------------------------
// Build a file name, based on a file type.
//----------------------------------------------------------------------------

ts::UString ts::SectionFile::BuildFileName(const UString& file_name, FileType type)
{
    switch (type) {
        case BINARY: return PathPrefix(file_name) + TS_DEFAULT_BINARY_SECTION_FILE_SUFFIX;
        case XML: return PathPrefix(file_name) + TS_DEFAULT_XML_SECTION_FILE_SUFFIX;
        default: return file_name;
    }
}


//----------------------------------------------------------------------------
// Load a binary or XML file.
//----------------------------------------------------------------------------

bool ts::SectionFile::load(const UString& file_name, Report& report, FileType type)
{
    switch (GetFileType(file_name, type)) {
        case BINARY:
            return loadBinary(file_name, report);
        case XML:
            return loadXML(file_name, report);
        default:
            report.error(u"unknown file type for %s", {file_name});
            return false;
    }
}

bool ts::SectionFile::load(std::istream& strm, Report& report, FileType type)
{
    switch (type) {
        case BINARY:
            return loadBinary(strm, report);
        case XML:
            return loadXML(strm, report);
        default:
            report.error(u"unknown input file type");
            return false;
    }
}

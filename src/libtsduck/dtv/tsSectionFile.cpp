//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsSectionFile.h"
#include "tsAbstractTable.h"
#include "tsAbstractDescriptor.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsSysUtils.h"
#include "tsEIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SectionFile::SectionFile(DuckContext& duck) :
    _duck(duck),
    _tables(),
    _sections(),
    _orphanSections(),
    _xmlTweaks(),
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
        table->serialize(_duck, *bin);
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
    if (!table.isNull()) {
        if (table->isValid()) {
            // The table is added as a whole.
            // Add the standards from the table in the context.
            _duck.addStandards(table->definingStandards());
            // Add the table as a whole.
            _tables.push_back(table);
            // Add all its sections (none of them is orphan).
            for (size_t i = 0; i < table->sectionCount(); ++i) {
                _sections.push_back(table->sectionAt(i));
            }
        }
        else {
            // The table is invalid. Add individual present sections.
            for (size_t i = 0; i < table->sectionCount(); ++i) {
                add(table->sectionAt(i)); // can be a null pointer
            }
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
        // Add the standards from the section in the context.
        _duck.addStandards(section->definingStandards());
        // Make the section part of the global list of sections.
        _sections.push_back(section);
        // Temporary push this section in the orphan list.
        _orphanSections.push_back(section);
        // Try to build a table from the list of orphans.
        collectLastTable();
    }
}


//----------------------------------------------------------------------------
// Pack all orphan sections.
//----------------------------------------------------------------------------

size_t ts::SectionFile::packOrphanSections()
{
    size_t createCount = 0;

    // Loop on all orphan sections, locating sets of sections from the same table.
    for (auto first = _orphanSections.begin(); first != _orphanSections.end(); ) {
        assert(!first->isNull());
        assert((*first)->isValid());

        // Point after first section.
        auto end = first + 1;

        // A short section should be a table in itself, no need to dive further.
        // Long sections must be grouped by tid / tid-ext.
        if ((*first)->isLongSection()) {
            const TID tid = (*first)->tableId();
            const uint16_t tidExt = (*first)->tableIdExtension();
            while (end != _orphanSections.end() && (*end)->tableId() == tid && (*end)->tableIdExtension() == tidExt) {
                ++end;
            }
        }

        // Build a binary table from orphan sections.
        BinaryTablePtr table(new BinaryTable);
        CheckNonNull(table.pointer());
        table->addSections(first, end, true, true);

        // Compress all sections to make a valid table.
        table->packSections();
        assert(table->isValid());

        // Now we got a table.
        _tables.push_back(table);
        createCount++;

        // Loop on next set of sections.
        first = end;
    }

    // Clear the list of orphan sections, they are now in tables.
    _orphanSections.clear();

    return createCount;
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
    SectionPtrVector::iterator first(_orphanSections.end());
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
// Reorganize all EIT sections according to ETSI TS 101 211.
//----------------------------------------------------------------------------

void ts::SectionFile::reorganizeEITs(const ts::Time& reftime)
{
    EIT::ReorganizeSections(_sections, reftime);
    rebuildTables();
}


//----------------------------------------------------------------------------
// Rebuild _tables and _orphanSections from _sections.
//----------------------------------------------------------------------------

void ts::SectionFile::rebuildTables()
{
    // Restart from scratch/
    _tables.clear();
    _orphanSections.clear();

    // Rebuild tables from consecutive sections.
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (_sections[i].isNull() || !_sections[i]->isValid()) {
            // Ignore invalid sections.
        }
        else if (_sections[i]->isShortSection()) {
            // Short sections are always full tables.
            _tables.push_back(new BinaryTable({_sections[i]}));
        }
        else if (_sections[i]->sectionNumber() != 0 || i + _sections[i]->lastSectionNumber() >= _sections.size()) {
            // Orphan section, not preceded by logically adjacent sections or section #0 without enough following sections.
            _orphanSections.push_back(_sections[i]);
        }
        else {
            // We have a long section #0, try to match all following sections.
            const TID tid = _sections[i]->tableId();
            const uint16_t tidext = _sections[i]->tableIdExtension();
            const size_t count = _sections[i]->lastSectionNumber() + 1;
            bool ok = true;
            SectionPtrVector secs;
            for (size_t i0 = 1; ok && i0 < count; ++i0) {
                secs.push_back(_sections[i + i0]);
                ok = _sections[i + i0]->tableId() == tid &&
                     _sections[i + i0]->tableIdExtension() == tidext &&
                     _sections[i + i0]->sectionNumber() == i0;
            }
            if (ok) {
                // All sections are present in order, this is a table.
                _tables.push_back(new BinaryTable(secs));
                i += count - 1;
            }
            else {
                // Cannot find a complete table. Push first section as orphan.
                _orphanSections.push_back(_sections[i]);
            }
        }
    }
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
// This static method loads the XML model for tables and descriptors.
//----------------------------------------------------------------------------

bool ts::SectionFile::LoadModel(xml::Document& doc)
{
    // Load the main model. Use searching rules.
    if (!doc.load(TS_XML_TABLES_MODEL, true)) {
        doc.report().error(u"Main model for TSDuck XML files not found: %s", {TS_XML_TABLES_MODEL});
        return false;
    }

    // Get the root element in the model.
    xml::Element* root = doc.rootElement();
    if (root == nullptr) {
        doc.report().error(u"Main model for TSDuck XML files is empty: %s", {TS_XML_TABLES_MODEL});
        return false;
    }

    // Get the list of all registered extension files.
    UStringList extfiles;
    PSIRepository::Instance()->getRegisteredTablesModels(extfiles);

    // Load all extension files. Only report a warning in case of failure.
    for (auto name = extfiles.begin(); name != extfiles.end(); ++name) {
        // Load the extension file. Use searching rules.
        xml::Document extdoc(doc.report());
        xml::Element* extroot = nullptr;
        xml::Element* elem = nullptr;
        if (!extdoc.load(*name, true)) {
            extdoc.report().error(u"Extension XML model file not found: %s", {*name});
        }
        else if ((extroot = extdoc.rootElement()) != nullptr) {
            // Remove elements one by one.
            while ((elem = extdoc.rootElement()->firstChildElement()) != nullptr) {
                if (!elem->name().startWith(u"_")) {
                    // The element does not start with an underscore.
                    // Simply move the element inside the main model.
                    elem->reparent(root);
                }
                else {
                    // The element starts with an underscore.
                    // We need to merge its content with an element of the same name in the model.
                    xml::Element* topic = root->findFirstChild(elem->name(), true);
                    if (topic == nullptr) {
                        // The topic did not exist in the main model, simply move is here.
                        elem->reparent(root);
                    }
                    else {
                        // Move all content into the main topic.
                        xml::Element* e = nullptr;
                        while ((e = elem->firstChildElement()) != nullptr) {
                            e->reparent(topic);
                        }
                        // Finally, delete the (now empty) element from the extension.
                        delete elem;
                    }
                }
            }
        }
    }

    return true;
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
    if (!LoadModel(model)) {
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
        if (bin->fromXML(_duck, node) && bin->isValid()) {
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
            table->toXML(_duck, root, false);
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
        case BINARY:
            return PathPrefix(file_name) + TS_DEFAULT_BINARY_SECTION_FILE_SUFFIX;
        case XML:
            return PathPrefix(file_name) + TS_DEFAULT_XML_SECTION_FILE_SUFFIX;
        case UNSPECIFIED:
        default:
            return file_name;
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
        case UNSPECIFIED:
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
        case UNSPECIFIED:
        default:
            report.error(u"unknown input file type");
            return false;
    }
}

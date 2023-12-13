//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSectionFile.h"
#include "tsSection.h"
#include "tsAbstractTable.h"
#include "tsBinaryTable.h"
#include "tsReportWithPrefix.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsxmlJSONConverter.h"
#include "tsjsonNull.h"
#include "tsFileUtils.h"
#include "tsEIT.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SectionFile::SectionFile(DuckContext& duck) :
    _duck(duck),
    _report(duck.report())
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
    for (auto& it : tables) {
        add(it);
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
    for (auto& it : sections) {
        add(it);
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
    auto first = _orphanSections.end();
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

void ts::SectionFile::reorganizeEITs(const ts::Time& reftime, EITOptions options)
{
    EIT::ReorganizeSections(_duck, _sections, reftime, options);
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

bool ts::SectionFile::loadBinary(const fs::path& file_name)
{
    // Separately process standard input.
    if (file_name.empty() || file_name == u"-") {
        return loadBinary(std::cin, _report);
    }

    // Open the input file.
    std::ifstream strm(file_name, std::ios::in | std::ios::binary);
    if (!strm.is_open()) {
        _report.error(u"cannot open %s", {file_name});
        return false;
    }

    // Load the section file.
    ReportWithPrefix report_internal(_report, file_name + u": ");
    const bool success = loadBinary(strm, report_internal);
    strm.close();

    return success;
}

bool ts::SectionFile::loadBinary(std::istream& strm, Report& report)
{
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

bool ts::SectionFile::saveBinary(const fs::path& file_name) const
{
    // Separately process standard output.
    if (file_name.empty() || file_name == u"-") {
        return saveBinary(std::cout, _report);
    }

    // Create the output file.
    std::ofstream strm(file_name, std::ios::out | std::ios::binary);
    if (!strm.is_open()) {
        _report.error(u"error creating %s", {file_name});
        return false;
    }

    // Save sections.
    ReportWithPrefix report_internal(_report, file_name + u": ");
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
// Load a binary section file from a memory buffer.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadBuffer(const void* buffer, size_t size)
{
    bool success = true;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
    while (size >= 3) {
        const size_t section_size = 3 + (GetUInt16(data + 1) & 0x0FFF);
        if (section_size > size) {
            break;
        }
        SectionPtr sp(new Section(data, section_size, PID_NULL, CRC32::CHECK));
        if (!sp.isNull() && sp->isValid()) {
            add(sp);
        }
        else {
            success = false;
        }
        data += section_size;
        size -= section_size;
    }
    return success && size == 0;
}

bool ts::SectionFile::loadBuffer(const ByteBlock& data, size_t start, size_t count)
{
    start = std::min(start, data.size());
    count = std::min(count, data.size() - start);
    return loadBuffer(data.data() + start, count);
}


//----------------------------------------------------------------------------
// Save the section file into a memory buffer.
//----------------------------------------------------------------------------

size_t ts::SectionFile::saveBuffer(void* buffer, size_t buffer_size) const
{
    size_t total = 0;
    if (buffer != nullptr) {
        uint8_t* data = reinterpret_cast<uint8_t*>(buffer);
        for (size_t i = 0; i < _sections.size(); ++i) {
            if (!_sections[i].isNull() && _sections[i]->isValid()) {
                const size_t size = _sections[i]->size();
                if (size > buffer_size) {
                    break;
                }
                std::memcpy(data, _sections[i]->content(), size);
                data += size;
                total += size;
                buffer_size -= size;
            }
        }
    }
    return total;
}

size_t ts::SectionFile::saveBuffer(ByteBlock& buffer) const
{
    // Pre-reserve memory to avoid reallocations.
    buffer.reserve(buffer.size() + binarySize());

    // Append all sections one by one.
    const size_t initial = buffer.size();
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (!_sections[i].isNull() && _sections[i]->isValid()) {
            buffer.append(_sections[i]->content(), _sections[i]->size());
        }
    }
    return buffer.size() - initial;
}


//----------------------------------------------------------------------------
// Get the size in bytes of all sections.
//----------------------------------------------------------------------------

size_t ts::SectionFile::binarySize() const
{
    size_t size = 0;
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (!_sections[i].isNull() && _sections[i]->isValid()) {
            size += _sections[i]->size();
        }
    }
    return size;
}


//----------------------------------------------------------------------------
// Load the XML model in this instance, if not already done.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadThisModel()
{
    if (_model.hasChildren()) {
        return true; // already loaded
    }
    else {
        _model.setTweaks(_xmlTweaks);
        return LoadModel(_model, true);
    }
}


//----------------------------------------------------------------------------
// This static method loads the XML model for tables and descriptors.
//----------------------------------------------------------------------------

bool ts::SectionFile::LoadModel(xml::Document& doc, bool load_extensions)
{
    // Load the main model. Use searching rules.
    if (!doc.load(XML_TABLES_MODEL, true)) {
        doc.report().error(u"Main model for TSDuck XML files not found: %s", {XML_TABLES_MODEL});
        return false;
    }

    // If no extension to be loaded, nothing more to do.
    if (!load_extensions) {
        return true;
    }

    // Get the root element in the model.
    xml::Element* root = doc.rootElement();
    if (root == nullptr) {
        doc.report().error(u"Main model for TSDuck XML files is empty: %s", {XML_TABLES_MODEL});
        return false;
    }

    // Get the list of all registered extension files.
    UStringList extfiles;
    PSIRepository::Instance().getRegisteredTablesModels(extfiles);

    // Load all extension files. Only report a warning in case of failure.
    for (const auto& name : extfiles) {
        // Load the extension file. Use searching rules.
        xml::Document extdoc(doc.report());
        if (!extdoc.load(name, true)) {
            extdoc.report().error(u"Extension XML model file not found: %s", {name});
        }
        else {
            root->merge(extdoc.rootElement());
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadXML(const UString& file_name)
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(file_name, false) && parseDocument(doc);
}

bool ts::SectionFile::loadXML(std::istream& strm)
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(strm) && parseDocument(doc);
}

bool ts::SectionFile::parseXML(const UString& xml_content)
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);
    return doc.parse(xml_content) && parseDocument(doc);
}

bool ts::SectionFile::parseDocument(const xml::Document& doc)
{
    // Load the XML model for TSDuck files, if not already done.
    if (!loadThisModel()) {
        return false;
    }

    // Validate the input document according to the model.
    if (!_model.validate(doc)) {
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

bool ts::SectionFile::saveXML(const UString& file_name) const
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) && doc.save(file_name);
}

ts::UString ts::SectionFile::toXML() const
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Load / parse a JSON file.
//----------------------------------------------------------------------------

bool ts::SectionFile::loadJSON(const UString& file_name)
{
    json::ValuePtr root;
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);

    return loadThisModel() &&
           json::LoadFile(root, file_name, _report) &&
           _model.convertToXML(*root, doc, true) &&
           parseDocument(doc);
}

bool ts::SectionFile::loadJSON(std::istream& strm)
{
    json::ValuePtr root;
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);

    return loadThisModel() &&
           json::LoadStream(root, strm, _report) &&
           _model.convertToXML(*root, doc, true) &&
           parseDocument(doc);
}

bool ts::SectionFile::parseJSON(const UString& json_content)
{
    json::ValuePtr root;
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);

    return loadThisModel() &&
           json::Parse(root, json_content, _report) &&
           _model.convertToXML(*root, doc, true) &&
           parseDocument(doc);
}


//----------------------------------------------------------------------------
// Create JSON file or text.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::SectionFile::convertToJSON()
{
    xml::Document doc(_report);
    doc.setTweaks(_xmlTweaks);

    // Load the XML model, generate the initial XML document, convert XML into JSON.
    if (loadThisModel() && generateDocument(doc)) {
        return _model.convertToJSON(doc);
    }
    else {
        return json::ValuePtr(new json::Null);
    }
}

bool ts::SectionFile::saveJSON(const UString& file_name)
{
    const json::ValuePtr root(convertToJSON());
    return !root->isNull() && root->save(file_name, 2, true, _report);
}

ts::UString ts::SectionFile::toJSON()
{
    const json::ValuePtr root(convertToJSON());
    if (root->isNull()) {
        return UString();
    }
    TextFormatter text(_report);
    text.setString();
    root->print(text);
    return text.toString();
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
    for (auto& table : _tables) {
        if (!table.isNull()) {
            table->toXML(_duck, root);
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
    if (type != FileType::UNSPECIFIED) {
        return type; // already known
    }
    if (xml::Document::IsInlineXML(file_name)) {
        return FileType::XML; // inline XML content
    }
    if (json::IsInlineJSON(file_name)) {
        return FileType::JSON; // inline JSON content
    }
    UString ext(fs::path(file_name).extension());
    ext.convertToLower();
    if (ext == DEFAULT_XML_SECTION_FILE_SUFFIX) {
        return FileType::XML;
    }
    if (ext == DEFAULT_JSON_SECTION_FILE_SUFFIX) {
        return FileType::JSON;
    }
    else if (ext == DEFAULT_BINARY_SECTION_FILE_SUFFIX) {
        return FileType::BINARY;
    }
    else {
        return FileType::UNSPECIFIED;
    }
}


//----------------------------------------------------------------------------
// Build a file name, based on a file type.
//----------------------------------------------------------------------------

fs::path ts::SectionFile::BuildFileName(const fs::path& file_name, FileType type)
{
    fs::path res(file_name);
    switch (type) {
        case FileType::BINARY:
            res.replace_extension(DEFAULT_BINARY_SECTION_FILE_SUFFIX);
            break;
        case FileType::XML:
            res.replace_extension(DEFAULT_XML_SECTION_FILE_SUFFIX);
            break;
        case FileType::JSON:
            res.replace_extension(DEFAULT_JSON_SECTION_FILE_SUFFIX);
            break;
        case FileType::UNSPECIFIED:
        default:
            break;
    }
    return res;
}


//----------------------------------------------------------------------------
// Load a binary or XML file.
//----------------------------------------------------------------------------

bool ts::SectionFile::load(const UString& file_name, FileType type)
{
    switch (GetFileType(file_name, type)) {
        case FileType::BINARY:
            return loadBinary(file_name);
        case FileType::XML:
            return loadXML(file_name);
        case FileType::JSON:
            return loadJSON(file_name);
        case FileType::UNSPECIFIED:
        default:
            _report.error(u"unknown file type for %s", {file_name});
            return false;
    }
}

bool ts::SectionFile::load(std::istream& strm, FileType type)
{
    switch (type) {
        case FileType::BINARY:
            return loadBinary(strm);
        case FileType::XML:
            return loadXML(strm);
        case FileType::JSON:
            return loadJSON(strm);
        case FileType::UNSPECIFIED:
        default:
            _report.error(u"unknown input file type");
            return false;
    }
}

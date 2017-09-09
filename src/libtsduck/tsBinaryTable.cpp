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
//  Representation of MPEG PSI/SI tables in binary form (ie. list of sections)
//
//----------------------------------------------------------------------------

#include "tsBinaryTable.h"
#include "tsReportWithPrefix.h"
#include "tsDecimal.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable () :
    _is_valid (false),
    _tid (0xFF),
    _tid_ext (0),
    _version (0),
    _source_pid (PID_NULL),
    _missing_count (0),
    _sections ()
{
}


//----------------------------------------------------------------------------
// Copy constructor. The sections are either shared between the
// two tables or duplicated.
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable (const BinaryTable& table, CopyShare mode) :
    _is_valid (table._is_valid),
    _tid (table._tid),
    _tid_ext (table._tid_ext),
    _version (table._version),
    _source_pid (table._source_pid),
    _missing_count (table._missing_count),
    _sections ()
{
    switch (mode) {
        case SHARE:
            // Copy the pointers, share the pointed sections
            _sections = table._sections;
            break;
        case COPY:
            _sections.resize (table._sections.size());
            for (size_t i = 0; i < _sections.size(); ++i) {
                if (table._sections[i].isNull()) {
                    _sections[i].clear();
                }
                else {
                    _sections[i] = new Section (*table._sections[i], COPY);
                }
            }
            break;
        default:
            // should not get there
            assert (false);
    }
}


//----------------------------------------------------------------------------
// Constructor from an array of sections.
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable (const SectionPtrVector& sections, bool replace, bool grow) :
    _is_valid (false),
    _tid (0xFF),
    _tid_ext (0),
    _version (0),
    _source_pid (PID_NULL),
    _missing_count (0),
    _sections ()
{
    if (!addSections (sections, replace, grow)) {
        clear();
    }
}


//----------------------------------------------------------------------------
// Assignment. The sections are referenced, and thus shared
// between the two table objects.
//----------------------------------------------------------------------------

ts::BinaryTable& ts::BinaryTable::operator= (const BinaryTable& table)
{
    _is_valid = table._is_valid;
    _tid = table._tid;
    _tid_ext = table._tid_ext;
    _version = table._version;
    _source_pid = table._source_pid;
    _missing_count = table._missing_count;
    _sections = table._sections;
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the sections are duplicated.
//----------------------------------------------------------------------------

ts::BinaryTable& ts::BinaryTable::copy (const BinaryTable& table)
{
    _is_valid = table._is_valid;
    _tid = table._tid;
    _tid_ext = table._tid_ext;
    _version = table._version;
    _source_pid = table._source_pid;
    _missing_count = table._missing_count;
    _sections.resize (table._sections.size());
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (table._sections[i].isNull()) {
            _sections[i].clear();
        }
        else {
            _sections[i] = new Section (*table._sections[i], COPY);
        }
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison. Note: Invalid tables are never identical
//----------------------------------------------------------------------------

bool ts::BinaryTable::operator== (const BinaryTable& table) const
{
    bool equal =
        _is_valid &&
        table._is_valid &&
        _tid == table._tid &&
        _tid_ext == table._tid_ext &&
        _version == table._version &&
        _sections.size() == table._sections.size();

    for (size_t i = 0; equal && i < _sections.size(); ++i) {
        equal = !_sections[i].isNull() && !table._sections[i].isNull() && *_sections[i] == *table._sections[i];
    }

    return equal;
}


//----------------------------------------------------------------------------
// Modifiable properties.
//----------------------------------------------------------------------------

void ts::BinaryTable::setTableIdExtension (uint16_t tid_ext, bool recompute_crc)
{
    _tid_ext = tid_ext;
    for (SectionPtrVector::iterator it = _sections.begin(); it != _sections.end(); ++it) {
        (*it)->setTableIdExtension (tid_ext, recompute_crc);
    }
}

void ts::BinaryTable::setVersion (uint8_t version, bool recompute_crc)
{
    _version = version;
    for (SectionPtrVector::iterator it = _sections.begin(); it != _sections.end(); ++it) {
        (*it)->setVersion (version, recompute_crc);
    }
}

void ts::BinaryTable::setSourcePID (PID pid)
{
    _source_pid = pid;
    for (SectionPtrVector::iterator it = _sections.begin(); it != _sections.end(); ++it) {
        (*it)->setSourcePID (pid);
    }
}


//----------------------------------------------------------------------------
// Index of first and last TS packet of the table in the demultiplexed stream.
//----------------------------------------------------------------------------

ts::PacketCounter ts::BinaryTable::getFirstTSPacketIndex() const
{
    bool found = false;
    PacketCounter first = std::numeric_limits<PacketCounter>::max();
    for (SectionPtrVector::const_iterator it = _sections.begin(); it != _sections.end(); ++it) {
        if (!it->isNull()) {
            found = true;
            first = std::min (first, (*it)->getFirstTSPacketIndex());
        }
    }
    return found ? first : 0;
}

ts::PacketCounter ts::BinaryTable::getLastTSPacketIndex() const
{
    PacketCounter last = 0;
    for (SectionPtrVector::const_iterator it = _sections.begin(); it != _sections.end(); ++it) {
        if (!it->isNull()) {
            last = std::max (last, (*it)->getLastTSPacketIndex());
        }
    }
    return last;
}


//----------------------------------------------------------------------------
// Clear the content of the table. The table must be rebuilt
// using calls to addSection.
//----------------------------------------------------------------------------

void ts::BinaryTable::clear ()
{
    _is_valid = false;
    _tid = 0xFF;
    _tid_ext = 0;
    _version = 0;
    _source_pid = PID_NULL;
    _missing_count = 0;
    _sections.clear ();
}


//----------------------------------------------------------------------------
// Return the total size in bytes of all sections in the table.
//----------------------------------------------------------------------------

size_t ts::BinaryTable::totalSize() const
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
// Add several sections to a table
//----------------------------------------------------------------------------

bool ts::BinaryTable::addSections(const SectionPtrVector& sections, bool replace, bool grow)
{
    bool ok = true;
    for (size_t n = 0; n < sections.size(); n++) {
        ok = addSection(sections[n], replace, grow) && ok;
    }
    return ok;
}


//----------------------------------------------------------------------------
// A table is built by adding sections using addSection.
// When all sections are present, the table becomes valid.
//----------------------------------------------------------------------------

bool ts::BinaryTable::addSection(const SectionPtr& sect, bool replace, bool grow)
{
    // Reject invalid sections

    if (!sect->isValid()) {
        return false;
    }

    // Check the compatibility of the section with the table

    const int index = sect->sectionNumber();

    if (_sections.size() == 0) {
        // This is the first section, set the various parameters
        _sections.resize(size_t(sect->lastSectionNumber()) + 1);
        assert(index < int(_sections.size()));
        _tid = sect->tableId();
        _tid_ext = sect->tableIdExtension();
        _version = sect->version();
        _source_pid = sect->sourcePID();
        _missing_count = int(_sections.size());
    }
    else if (sect->tableId() != _tid || sect->tableIdExtension() != _tid_ext || sect->version() != _version) {
        // Not the same table
        return false;
    }
    else if (!grow && (index >= int(_sections.size()) || size_t(sect->lastSectionNumber()) != _sections.size() - 1)) {
        // Incompatible number of sections
        return false;
    }
    else if (size_t(sect->lastSectionNumber()) != _sections.size() - 1) {
        // Incompatible number of sections but the table is allowed to grow
        if (size_t(sect->lastSectionNumber()) < _sections.size() - 1) {
            // The new section must be updated
            sect->setLastSectionNumber(uint8_t(int(_sections.size()) - 1));
        }
        else {
            // The table must be updated (more sections)
            _missing_count += int(sect->lastSectionNumber()) + 1 - int(_sections.size());
            _sections.resize(size_t(sect->lastSectionNumber()) + 1);
            assert(index < int(_sections.size()));
            // Modify all previously entered sections
            for (int si = 0; si < int(_sections.size()); ++si) {
                if (!_sections[si].isNull()) {
                    _sections[si]->setLastSectionNumber(sect->lastSectionNumber());
                }
            }
        }
    }

    // Now add the section

    if (_sections[index].isNull()) {
        // The section was not present, add it
        _sections[index] = sect;
        _missing_count--;
    }
    else if (!replace) {
        // Section already present, don't replace
        return false;
    }
    else {
        // Section already present but replace
        _sections[index] = sect;
    }

    // The table becomes valid if there is no more missing section

    _is_valid = _missing_count == 0;
    assert(_missing_count >= 0);

    return true;
}


//----------------------------------------------------------------------------
// Check if this is a table with one short section.
//----------------------------------------------------------------------------

bool ts::BinaryTable::isShortSection() const
{
    return _sections.size() == 1 && !_sections[0].isNull() && _sections[0]->isShortSection();
}


//----------------------------------------------------------------------------
// Write the binary table on standard streams.
//----------------------------------------------------------------------------

std::ostream& ts::BinaryTable::write(std::ostream& strm, ReportInterface& report) const
{
    if (!_is_valid) {
        report.error("invalid table, cannot write it to file");
        // Flawfinder: ignore: completely fooled here, std::ostream::setstate has nothing to do with PRNG.
        strm.setstate(std::ios::failbit);
    }
    else {
        // Write all sections to the file
        for (size_t n = 0; n < _sections.size() && _sections[n]->write(strm, report); ++n) {
        }
    }
    return strm;
}


//----------------------------------------------------------------------------
// Save the binary table in a file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::BinaryTable::save(const std::string& file_name, ReportInterface& report) const
{
    std::ofstream outfile(file_name.c_str(), std::ios::out | std::ios::binary);
    if (!outfile) {
        report.error ("error creating %s", file_name.c_str());
        return false;
    }
    else {
        ReportWithPrefix report_internal(report, file_name + ": ");
        write(outfile, report_internal);
        const bool ok = outfile.good();
        outfile.close();
        return ok;
    }
}


//----------------------------------------------------------------------------
// Error message fragment indicating the number of bytes previously
// read in a binary file
//----------------------------------------------------------------------------

namespace {
    std::string AfterBytes(const std::streampos& position)
    {
        const int64_t bytes = int64_t (position);
        if (bytes > 0) {
            return " after " + ts::Decimal (bytes) + " bytes";
        }
        else {
            return "";
        }
    }
}

//----------------------------------------------------------------------------
// This static method reads all tables from the specified file.
//----------------------------------------------------------------------------

bool ts::BinaryTable::LoadFile(BinaryTablePtrVector& tables, std::istream& strm, CRC32::Validation crc_op, ReportInterface& report)
{
    tables.clear();

    BinaryTablePtr tp;
    uint8_t next_section = 0; // expected section number
    for (;;) {

        // Read one section.
        std::streampos position(strm.tellg());
        SectionPtr sp(new Section);
        if (!sp->read(strm, crc_op, report)) {
            return false;
        }

        // Check the sequence of section numbers.
        if (sp->sectionNumber() != next_section) {
            report.error(Format("invalid section number, got %d, expected %d", int(sp->sectionNumber()), int(next_section)) + AfterBytes(position));
            return false;
        }

        // Add the section in the current table.
        if (tp.isNull()) {
            // This is the first section of a table.
            tp = new BinaryTable;
        }
        if (!tp->addSection(sp, false, false)) {
            report.error("invalid section" + AfterBytes(position));
            return false;
        }

        // If the table is complete, add it in the returned list.
        if (tp->isValid()) {
            tables.push_back(tp);
            tp.clear();
            next_section = 0;
        }
        else {
            next_section++;
        }
    }

    // Check that the last table is complete.
    if (!tp.isNull()) {
        report.error("truncated table at end of file");
        return false;
    }

    // Success if reached EOF without error
    return strm.eof();
}


//----------------------------------------------------------------------------
// This static method reads all tables from the specified file.
//----------------------------------------------------------------------------

bool ts::BinaryTable::LoadFile(BinaryTablePtrVector& tables, const std::string& file_name, CRC32::Validation crc_op, ReportInterface& report)
{
    tables.clear();

    // Open the input file.
    std::ifstream strm(file_name.c_str(), std::ios::in | std::ios::binary);
    if (!strm.is_open()) {
        report.error("cannot open " + file_name);
        return false;
    }

    // Load the section file
    ReportWithPrefix report_internal(report, file_name + ": ");
    const bool success = LoadFile(tables, strm, crc_op, report_internal);
    strm.close();
    return success;
}


//----------------------------------------------------------------------------
// This static method writes all tables to the specified file.
//----------------------------------------------------------------------------

std::ostream& ts::BinaryTable::SaveFile(const BinaryTablePtrVector& tables, std::ostream& strm, ReportInterface& report)
{
    for (size_t i = 0; i < tables.size() && strm.good(); ++i) {
        if (!tables[i].isNull()) {
            tables[i]->write(strm, report);
        }
    }
    return strm;
}


//----------------------------------------------------------------------------
// This static method writes all tables to the specified file.
//----------------------------------------------------------------------------

bool ts::BinaryTable::SaveFile(const BinaryTablePtrVector& tables, const std::string& file_name, ReportInterface& report)
{
    std::ofstream outfile(file_name.c_str(), std::ios::out | std::ios::binary);
    if (!outfile) {
        report.error("error creating %s", file_name.c_str());
        return false;
    }
    else {
        ReportWithPrefix report_internal(report, file_name + ": ");
        SaveFile(tables, outfile, report_internal);
        const bool ok = outfile.good();
        outfile.close();
        return ok;
    }
}

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
//  Representation of MPEG PSI/SI sections
//
//----------------------------------------------------------------------------

#include "tsSection.h"
#include "tsDecimal.h"
#include "tsCRC32.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsNames.h"
#include "tsHexa.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::Section::Section() :
    _is_valid(false),
    _source_pid(PID_NULL),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
}


//----------------------------------------------------------------------------
// Copy constructor. The section content is either shared or referenced.
//----------------------------------------------------------------------------

ts::Section::Section(const Section& sect, CopyShare mode) :
    _is_valid(sect._is_valid),
    _source_pid(sect._source_pid),
    _first_pkt(sect._first_pkt),
    _last_pkt(sect._last_pkt),
    _data()
{
    switch (mode) {
        case SHARE:
            _data = sect._data;
            break;
        case COPY:
            _data = sect._is_valid ? new ByteBlock (*sect._data) : 0;
            break;
        default:
            // should not get there
            assert (false);
    }
}


//----------------------------------------------------------------------------
// Constructor from full binary content.
//----------------------------------------------------------------------------

ts::Section::Section(const void* content,
                     size_t content_size,
                     PID source_pid,
                     CRC32::Validation crc_op) :
    _is_valid(false),
    _source_pid(source_pid),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
    initialize(new ByteBlock(content, content_size), source_pid, crc_op);
}


//----------------------------------------------------------------------------
// Constructor from full binary content.
//----------------------------------------------------------------------------

ts::Section::Section(const ByteBlock& content,
                     PID source_pid,
                     CRC32::Validation crc_op) :
    _is_valid(false),
    _source_pid(source_pid),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
    initialize(new ByteBlock(content), source_pid, crc_op);
}


//----------------------------------------------------------------------------
// Constructor from full binary content.
//----------------------------------------------------------------------------

ts::Section::Section(const ByteBlockPtr& content_ptr,
                     PID source_pid,
                     CRC32::Validation crc_op) :
    _is_valid(false),
    _source_pid(source_pid),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
    initialize(content_ptr, source_pid, crc_op);
}


//----------------------------------------------------------------------------
// Constructor from a short section payload.
//----------------------------------------------------------------------------

ts::Section::Section(TID tid,
                     bool is_private_section,
                     const void* payload,
                     size_t payload_size,
                     PID source_pid) :
    _is_valid(false),
    _source_pid(source_pid),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
    reload(tid, is_private_section, payload, payload_size, source_pid);
}


//----------------------------------------------------------------------------
// Constructor from a long section payload.
//----------------------------------------------------------------------------

ts::Section::Section(TID tid,
                     bool is_private_section,
                     uint16_t tid_ext,
                     uint8_t version,
                     bool is_current,
                     uint8_t section_number,
                     uint8_t last_section_number,
                     const void* payload,
                     size_t payload_size,
                     PID source_pid) :
    _is_valid(false),
    _source_pid(source_pid),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
    reload(tid, is_private_section, tid_ext, version, is_current,
           section_number, last_section_number,
           payload, payload_size, source_pid);
}


//----------------------------------------------------------------------------
// Reload short section
//----------------------------------------------------------------------------

void ts::Section::reload(TID tid,
                         bool is_private_section,
                         const void* payload,
                         size_t payload_size,
                         PID source_pid)
{
    initialize (source_pid);
    _is_valid = SHORT_SECTION_HEADER_SIZE + payload_size <= MAX_PRIVATE_SECTION_SIZE;
    _data = new ByteBlock (SHORT_SECTION_HEADER_SIZE + payload_size);
    PutUInt8 (_data->data(), tid);
    PutUInt16 (_data->data() + 1, (is_private_section ? 0x4000 : 0x0000) | 0x3000 | uint16_t (payload_size & 0x0FFF));
    ::memcpy (_data->data() + 3, payload, payload_size);
}


//----------------------------------------------------------------------------
// Reload long section
// The provided payload does not contain the CRC32.
// The CRC32 is automatically computed.
//----------------------------------------------------------------------------

void ts::Section::reload(TID tid,
                         bool is_private_section,
                         uint16_t tid_ext,
                         uint8_t version,
                         bool is_current,
                         uint8_t section_number,
                         uint8_t last_section_number,
                         const void* payload,
                         size_t payload_size,
                         PID source_pid)
{
    initialize (source_pid);
    _is_valid = section_number <= last_section_number && version <= 31 &&
        LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE <= MAX_PRIVATE_SECTION_SIZE;
    _data = new ByteBlock (LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE);
    PutUInt8 (_data->data(), tid);
    PutUInt16 (_data->data() + 1,
               0x8000 | (is_private_section ? 0x4000 : 0x0000) | 0x3000 |
               uint16_t ((LONG_SECTION_HEADER_SIZE - 3 + payload_size + SECTION_CRC32_SIZE) & 0x0FFF));
    PutUInt16 (_data->data() + 3, tid_ext);
    PutUInt8 (_data->data() + 5, 0xC0 | ((version & 0x1F) << 1) | (is_current ? 0x01 : 0x00));
    PutUInt8 (_data->data() + 6, section_number);
    PutUInt8 (_data->data() + 7, last_section_number);
    ::memcpy (_data->data() + 8, payload, payload_size);
    recomputeCRC ();
}


//----------------------------------------------------------------------------
// Private method: Helper for constructors.
//----------------------------------------------------------------------------

void ts::Section::initialize(PID pid)
{
    _is_valid = false;
    _source_pid = pid;
    _first_pkt = 0;
    _last_pkt = 0;
    _data = 0;
}


//----------------------------------------------------------------------------
// Private method: Helper for constructors.
//----------------------------------------------------------------------------

void ts::Section::initialize(const ByteBlockPtr& bbp, PID pid, CRC32::Validation crc_op)
{
    initialize (pid);
    _data = bbp;

    // Basic check, for min and max section size

    _is_valid = _data->size() >= MIN_SHORT_SECTION_SIZE && _data->size() <= MAX_PRIVATE_SECTION_SIZE;

    // Extract short section header info

    if (_is_valid) {
        uint16_t length = GetUInt16 (&(*_data)[1]) & 0x0FFF;
        _is_valid = length == _data->size() - 3;
    }

    // Extract long section header info

    if (isLongSection()) {
        _is_valid = _data->size() >= MIN_LONG_SECTION_SIZE && sectionNumber() <= lastSectionNumber();
    }

    // Check CRC32 if required

    if (isLongSection()) {
        const size_t size = _data->size() - 4;
        switch (crc_op) {
            case CRC32::CHECK:
                _is_valid = CRC32 (_data->data(), size) == GetUInt32 (&(*_data)[size]);
                break;
            case CRC32::COMPUTE:
                PutUInt32 (_data->data() + size, CRC32 (_data->data(), size).value());
                break;
            case CRC32::IGNORE:
                break;
            default:
                break;
        }
    }

    if (!_is_valid) {
        _data = 0;
    }
}


//----------------------------------------------------------------------------
// Assignment. The section content is referenced, and thus shared
// between the two section objects.
//----------------------------------------------------------------------------

ts::Section& ts::Section::operator= (const Section& sect)
{
    _is_valid = sect._is_valid;
    _source_pid = sect._source_pid;
    _first_pkt = sect._first_pkt;
    _last_pkt = sect._last_pkt;
    _data = sect._data;
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the section
// is duplicated.
//----------------------------------------------------------------------------

ts::Section& ts::Section::copy (const Section& sect)
{
    _is_valid = sect._is_valid;
    _source_pid = sect._source_pid;
    _first_pkt = sect._first_pkt;
    _last_pkt = sect._last_pkt;
    _data = sect._is_valid ? new ByteBlock (*sect._data) : 0;
    return *this;
}


//----------------------------------------------------------------------------
// Comparison. Note: Invalid sections are never identical
//----------------------------------------------------------------------------

bool ts::Section::operator== (const Section& sect) const
{
    return _is_valid && sect._is_valid && (_data == sect._data || *_data == *sect._data);
}


//----------------------------------------------------------------------------
// This method recomputes and replaces the CRC32 of the section.
//----------------------------------------------------------------------------

void ts::Section::recomputeCRC ()
{
    if (isLongSection()) {
        const size_t size = _data->size() - 4;
        PutUInt32 (_data->data() + size, CRC32 (_data->data(), size).value());
    }
}


//----------------------------------------------------------------------------
// Modifiable properties.
//----------------------------------------------------------------------------

void ts::Section::setTableIdExtension (uint16_t tid_ext, bool recompute_crc)
{
    if (isLongSection()) {
        PutUInt16 (_data->data() + 3, tid_ext);
        if (recompute_crc) {
            recomputeCRC ();
        }
    }
}

void ts::Section::setVersion (uint8_t version, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[5] = ((*_data)[5] & 0xC1) | ((version & 0x1F) << 1);
        if (recompute_crc) {
            recomputeCRC ();
        }
    }
}

void ts::Section::setIsCurrent (bool is_current, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[5] = ((*_data)[5] & 0xFE) | (is_current ? 0x01 : 0x00);
        if (recompute_crc) {
            recomputeCRC ();
        }
    }
}

void ts::Section::setSectionNumber (uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[6] = num;
        if (recompute_crc) {
            recomputeCRC ();
        }
    }
}

void ts::Section::setLastSectionNumber (uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[7] = num;
        if (recompute_crc) {
            recomputeCRC ();
        }
    }
}


//----------------------------------------------------------------------------
// Write section on standard streams.
//----------------------------------------------------------------------------

std::ostream& ts::Section::write (std::ostream& strm, ReportInterface& report) const
{
    if (_is_valid && strm) {
        strm.write (reinterpret_cast <const char*> (_data->data()), std::streamsize (_data->size()));
        if (!strm) {
            report.error ("error writing section into binary stream");
        }
    }
    return strm;
}


//----------------------------------------------------------------------------
// Error message fragment indicating the number of bytes previously
// read in a binary file
//----------------------------------------------------------------------------

namespace {
    std::string AfterBytes (const std::streampos& position)
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
// Read section from a stream. If a section is invalid (eof before end of
// section, wrong crc), the failbit of the stream is set.
//----------------------------------------------------------------------------

std::istream& ts::Section::read(std::istream& strm, CRC32::Validation crc_op, ReportInterface& report)
{
    // Invalidate current content
    clear();

    // If file already in error, nothing to do
    if (!strm) {
        return strm;
    }

    // Section size and content
    size_t secsize = 3; // short header size
    ByteBlockPtr secdata(0);

    // Read short header
    uint8_t header[3];
    std::streampos position(strm.tellg());
    strm.read(reinterpret_cast <char*> (header), 3);
    size_t insize = size_t(strm.gcount());

    // Read rest of the section
    if (insize == 3) {
        secsize += GetUInt16(header + 1) & 0x0FFF;
        secdata = new ByteBlock(secsize);
        CheckNonNull(secdata.pointer());
        ::memcpy(secdata->data(), header, 3);
        strm.read(reinterpret_cast <char*> (secdata->data() + 3), std::streamsize(secsize - 3));
        insize += size_t(strm.gcount());
    }

    if (insize != secsize) {
        // Truncated section
        if (insize > 0) {
            strm.setstate(std::ios::failbit);
            report.error("truncated section" + AfterBytes(position) +
                         ", got " + Decimal(insize) + " bytes, expected " + Decimal(secsize));
        }
    }
    else {
        // Section fully read
        reload(secdata, PID_NULL, crc_op);
        if (!_is_valid) {
            report.error("invalid section" + AfterBytes(position));
        }
    }

    return strm;
}


//----------------------------------------------------------------------------
// This static method reads all sections from the specified file.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Section::LoadFile(SectionPtrVector& sections,
                           std::istream& strm,
                           CRC32::Validation crc_op,
                           ReportInterface& report)
{
    sections.clear();
    for (;;) {
        SectionPtr sp (new Section ());
        if (sp->read(strm, crc_op, report)) {
            sections.push_back(sp);
        }
        else {
            break;
        }
    }

    // Success if reached EOF without error
    return strm.eof();
}


//----------------------------------------------------------------------------
// This static method reads all sections from the specified file.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Section::LoadFile(SectionPtrVector& sections,
                           const std::string& file_name,
                           CRC32::Validation crc_op,
                           ReportInterface& report)
{
    // Open the input file

    std::ifstream strm (file_name.c_str(), std::ios::in | std::ios::binary);

    if (!strm.is_open()) {
        report.error("cannot open " + file_name);
        return false;
    }

    // This internal class reports the messages with file name added.

    class ReportWithName: public ReportInterface
    {
    private:
        const std::string& _name;
        ReportInterface&   _report;
    public:
        // Constructor
        ReportWithName(const std::string& name, ReportInterface& rep) :
            _name(name),
            _report(rep)
        {
        }
    protected:
        // Logger
        virtual void writeLog(int severity, const std::string& msg)
        {
            _report.log(severity, _name + ": " + msg);
        }
    };

    // Load the section file

    ReportWithName report_internal(file_name, report);
    bool success = LoadFile(sections, strm, crc_op, report_internal);
    strm.close();

    return success;
}


//----------------------------------------------------------------------------
// Dump the section on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::Section::dump(std::ostream& strm, int indent, CASFamily cas, bool no_header) const
{
    const std::string margin(indent, ' ');
    const TID tid(tableId());

    // Filter invalid section
    if (!_is_valid) {
        return strm;
    }

    // Display common header lines.
    // If PID is the null PID, this means "unknown PID"
    if (!no_header) {
        strm << margin << "* Section dump"
             << Format(", PID %d (0x%04X)", int(_source_pid), int(_source_pid))
             << Format(", TID %d (0x%02X)", tid, tid)
             << " (" << names::TID(tid, cas) << ")" << std::endl
             << margin << "  Section size: " << size()
             << " bytes, header: " << (isLongSection() ? "long" : "short") << std::endl;
        if (isLongSection()) {
            strm << margin
                 << Format("  TIDext: %d (0x%04X)", int(tableIdExtension()), int(tableIdExtension()))
                 << ", version: " << int(version())
                 << ", index: " << int(sectionNumber())
                 << ", last: " << int(lastSectionNumber())
                 << ", " << (isNext() ? "next" : "current") << std::endl;
        }
    }

    // Display section body
    strm << Hexa(content(), size(), hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent + 2);
    return strm;
}

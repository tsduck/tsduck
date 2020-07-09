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

#include "tsSection.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsCRC32.h"
#include "tsNames.h"
#include "tsMemory.h"
#include "tsReportWithPrefix.h"
TSDUCK_SOURCE;


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

ts::Section::Section(const Section& sect, ShareMode mode) :
    _is_valid(sect._is_valid),
    _source_pid(sect._source_pid),
    _first_pkt(sect._first_pkt),
    _last_pkt(sect._last_pkt),
    _data()
{
    switch (mode) {
        case ShareMode::SHARE:
            _data = sect._data;
            break;
        case ShareMode::COPY:
            _data = sect._is_valid ? new ByteBlock (*sect._data) : nullptr;
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
    initialize(source_pid);
    _is_valid = SHORT_SECTION_HEADER_SIZE + payload_size <= MAX_PRIVATE_SECTION_SIZE;
    _data = new ByteBlock(SHORT_SECTION_HEADER_SIZE + payload_size);
    PutUInt8(_data->data(), tid);
    PutUInt16(_data->data() + 1, (is_private_section ? 0x4000 : 0x0000) | 0x3000 | uint16_t (payload_size & 0x0FFF));
    ::memcpy(_data->data() + 3, payload, payload_size);  // Flawfinder: ignore: memcpy()
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
    initialize(source_pid);
    _is_valid = section_number <= last_section_number && version <= 31 &&
        LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE <= MAX_PRIVATE_SECTION_SIZE;
    _data = new ByteBlock(LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE);
    PutUInt8(_data->data(), tid);
    PutUInt16(_data->data() + 1,
              0x8000 | (is_private_section ? 0x4000 : 0x0000) | 0x3000 |
              uint16_t((LONG_SECTION_HEADER_SIZE - 3 + payload_size + SECTION_CRC32_SIZE) & 0x0FFF));
    PutUInt16(_data->data() + 3, tid_ext);
    PutUInt8(_data->data() + 5, 0xC0 | uint8_t((version & 0x1F) << 1) | (is_current ? 0x01 : 0x00));
    PutUInt8(_data->data() + 6, section_number);
    PutUInt8(_data->data() + 7, last_section_number);
    ::memcpy(_data->data() + 8, payload, payload_size);  // Flawfinder: ignore: memcpy()
    recomputeCRC();
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
    _data = nullptr;
}


//----------------------------------------------------------------------------
// Static method to compute a section size. Return zero on error.
//----------------------------------------------------------------------------

size_t ts::Section::SectionSize(const void* content, size_t content_size)
{
    if (content == nullptr || content_size < MIN_SHORT_SECTION_SIZE || content_size > MAX_PRIVATE_SECTION_SIZE) {
        return 0;
    }
    else {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(content);
        size_t length = 3 + (GetUInt16(data + 1) & 0x0FFF);
        return length < content_size ? 0 : length;
    }
}


//----------------------------------------------------------------------------
// Private method: Helper for constructors.
//----------------------------------------------------------------------------

void ts::Section::initialize(const ByteBlockPtr& bbp, PID pid, CRC32::Validation crc_op)
{
    initialize(pid);
    _data = bbp;

    // Basic validity check using section size
    const size_t total_size = SectionSize(*bbp);
    _is_valid = total_size > 0 && total_size == _data->size();

    // Extract long section header info
    if (isLongSection()) {
        _is_valid = _data->size() >= MIN_LONG_SECTION_SIZE && sectionNumber() <= lastSectionNumber();
    }

    // Check CRC32 if required
    if (isLongSection()) {
        // Section size, without CRC32:
        const size_t size = _data->size() - 4;
        switch (crc_op) {
            case CRC32::CHECK:
                _is_valid = CRC32(_data->data(), size) == GetUInt32(&(*_data)[size]);
                break;
            case CRC32::COMPUTE:
                PutUInt32(_data->data() + size, CRC32(_data->data(), size).value());
                break;
            case CRC32::IGNORE:
                break;
            default:
                break;
        }
    }

    if (!_is_valid) {
        _data = nullptr;
    }
}


//----------------------------------------------------------------------------
// Assignment. The section content is referenced, and thus shared
// between the two section objects.
//----------------------------------------------------------------------------

ts::Section& ts::Section::operator=(const Section& sect)
{
    if (&sect != this) {
        _is_valid = sect._is_valid;
        _source_pid = sect._source_pid;
        _first_pkt = sect._first_pkt;
        _last_pkt = sect._last_pkt;
        _data = sect._data;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the section
// is duplicated.
//----------------------------------------------------------------------------

ts::Section& ts::Section::copy(const Section& sect)
{
    if (&sect != this) {
        _is_valid = sect._is_valid;
        _source_pid = sect._source_pid;
        _first_pkt = sect._first_pkt;
        _last_pkt = sect._last_pkt;
        _data = sect._is_valid ? new ByteBlock(*sect._data) : nullptr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison. Note: Invalid sections are never identical
//----------------------------------------------------------------------------

bool ts::Section::operator==(const Section& sect) const
{
    return _is_valid && sect._is_valid && (_data == sect._data || *_data == *sect._data);
}


//----------------------------------------------------------------------------
// This method recomputes and replaces the CRC32 of the section.
//----------------------------------------------------------------------------

void ts::Section::recomputeCRC()
{
    if (isLongSection()) {
        const size_t size = _data->size() - 4;
        PutUInt32(_data->data() + size, CRC32(_data->data(), size).value());
    }
}


//----------------------------------------------------------------------------
// Implementation of AbstractDefinedByStandards.
//----------------------------------------------------------------------------

ts::Standards ts::Section::definingStandards() const
{
    // The defining standard is taken from table id.
    return PSIRepository::Instance()->getTableStandards(tableId(), _source_pid);
}


//----------------------------------------------------------------------------
// Check if a data area of at least 3 bytes can be the start of a long section.
//----------------------------------------------------------------------------

bool ts::Section::StartLongSection(const uint8_t* data, size_t size)
{
    // According to MPEG, a long section has bit section_syntax_indicator set to 1.
    // However, the DVB spec is incompatible with MPEG for the Stuffing Table (ST).
    // In a DVB-ST, the section is always a short one, regardless of the section_syntax_indicator.
    return data != nullptr && size >= MIN_SHORT_SECTION_SIZE && (data[1] & 0x80) != 0 && data[0] != TID_ST;
}


//----------------------------------------------------------------------------
// Check if the section has a "diversified" payload.
//----------------------------------------------------------------------------

bool ts::Section::hasDiversifiedPayload() const
{
    return _is_valid && !IdenticalBytes(payload(), payloadSize());
}


//----------------------------------------------------------------------------
// Modifiable properties.
//----------------------------------------------------------------------------

void ts::Section::setTableIdExtension(uint16_t tid_ext, bool recompute_crc)
{
    if (isLongSection()) {
        PutUInt16(_data->data() + 3, tid_ext);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setVersion(uint8_t version, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[5] = ((*_data)[5] & 0xC1) | uint8_t((version & 0x1F) << 1);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setIsCurrent(bool is_current, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[5] = ((*_data)[5] & 0xFE) | (is_current ? 0x01 : 0x00);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setSectionNumber(uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[6] = num;
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setLastSectionNumber(uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        (*_data)[7] = num;
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setUInt8(size_t offset, uint8_t value, bool recompute_crc)
{
    if (_is_valid && offset < payloadSize()) {
        PutUInt8(_data->data() + headerSize() + offset, value);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setUInt16(size_t offset, uint16_t value, bool recompute_crc)
{
    if (_is_valid && offset + 1 < payloadSize()) {
        PutUInt16(_data->data() + headerSize() + offset, value);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}


//----------------------------------------------------------------------------
// Append binary data to the payload of the section.
//----------------------------------------------------------------------------

void ts::Section::appendPayload(const void* data, size_t size, bool recompute_crc)
{
    if (_is_valid && data != nullptr && size != 0) {
        // Update section size in header.
        PutUInt16(_data->data() + 1, (GetUInt16(_data->data() + 1) & 0xF000) | uint16_t((_data->size() + size - 3) & 0x0FFF));

        // Remove trailing CRC (now invalid) at end of long section.
        const bool is_long = isLongSection() && _data->size() >= LONG_SECTION_HEADER_SIZE + 4;
        if (is_long) {
            _data->resize(_data->size() - 4);
        }

        // Append the data.
        _data->append(data, size);

        // Restore a trailing CRC at end of long section and optionally recompute it.
        if (is_long) {
            _data->appendUInt32(0);
            if (recompute_crc) {
                recomputeCRC();
            }
        }
    }
}


//----------------------------------------------------------------------------
// Write section on standard streams.
//----------------------------------------------------------------------------

std::ostream& ts::Section::write(std::ostream& strm, Report& report) const
{
    if (_is_valid && strm) {
        strm.write(reinterpret_cast <const char*> (_data->data()), std::streamsize(_data->size()));
        if (!strm) {
            report.error(u"error writing section into binary stream");
        }
    }
    return strm;
}


//----------------------------------------------------------------------------
// Read section from a stream. If a section is invalid (eof before end of
// section, wrong crc), the failbit of the stream is set.
//----------------------------------------------------------------------------

std::istream& ts::Section::read(std::istream& strm, CRC32::Validation crc_op, Report& report)
{
    // Invalidate current content
    clear();

    // If file already in error, nothing to do
    if (!strm) {
        return strm;
    }

    // Section size and content
    size_t secsize = 3;  // short header size
    ByteBlockPtr secdata;

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
        ::memcpy(secdata->data(), header, 3);  // Flawfinder: ignore: memcpy()
        strm.read(reinterpret_cast <char*>(secdata->data() + 3), std::streamsize(secsize - 3));
        insize += size_t(strm.gcount());
    }

    if (insize != secsize) {
        // Truncated section
        if (insize > 0) {
            // Flawfinder: ignore: completely fooled here, std::ostream::setstate has nothing to do with PRNG.
            strm.setstate(std::ios::failbit);
            report.error(u"truncated section%s, got %d bytes, expected %d", {UString::AfterBytes(position), insize, secsize});
        }
    }
    else {
        // Section fully read
        reload(secdata, PID_NULL, crc_op);
        if (!_is_valid) {
            strm.setstate(std::ios::failbit);
            report.error(u"invalid section%s", {UString::AfterBytes(position)});
        }
    }

    return strm;
}


//----------------------------------------------------------------------------
// Dump the section on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::Section::dump(std::ostream& strm, int indent, uint16_t cas, bool no_header) const
{
    const std::string margin(indent, ' ');
    const TID tid(tableId());

    // Build a fake context based on the standards which define this section.
    DuckContext duck;
    duck.addStandards(definingStandards());

    // Filter invalid section
    if (!_is_valid) {
        return strm;
    }

    // Display common header lines.
    // If PID is the null PID, this means "unknown PID"
    if (!no_header) {
        strm << margin << ""
             << UString::Format(u"* Section dump, PID 0x%X (%d), TID %d", {_source_pid, _source_pid, names::TID(duck, tid, cas, names::BOTH_FIRST)})
             << std::endl
             << margin << "  Section size: " << size() << " bytes, header: " << (isLongSection() ? "long" : "short")
             << std::endl;
        if (isLongSection()) {
            strm << margin
                 << UString::Format(u"  TIDext: 0x%X (%d), version: %d, index: %d, last: %d, %s",
                                    {tableIdExtension(), tableIdExtension(),
                                     version(), sectionNumber(), lastSectionNumber(),
                                     (isNext() ? u"next" : u"current")})
                 << std::endl;
        }
    }

    // Display section body
    strm << UString::Dump(content(), size(), UString::HEXA | UString::ASCII | UString::OFFSET, indent + 2);
    return strm;
}

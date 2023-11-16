//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSection.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsCRC32.h"
#include "tsSHA1.h"
#include "tsNames.h"
#include "tsMemory.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::Section::Section(const Section& sect, ShareMode mode) :
    SuperClass(sect, mode),
    _is_valid(sect._is_valid)
{
}

ts::Section::Section(const void* content, size_t content_size, PID source_pid, CRC32::Validation crc_op) :
    SuperClass(content, content_size, source_pid),
    _is_valid(false)
{
    validate(crc_op);
}

ts::Section::Section(const ByteBlock& content, PID source_pid, CRC32::Validation crc_op) :
    SuperClass(content, source_pid),
    _is_valid(false)
{
    validate(crc_op);
}

ts::Section::Section(const ByteBlockPtr& content_ptr, PID source_pid, CRC32::Validation crc_op) :
    SuperClass(content_ptr, source_pid),
    _is_valid(false)
{
    validate(crc_op);
}


//----------------------------------------------------------------------------
// Constructor from a short section payload.
//----------------------------------------------------------------------------

ts::Section::Section(TID tid, bool is_private_section, const void* payload, size_t payload_size, PID source_pid) :
    Section()
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
    Section()
{
    reload(tid, is_private_section, tid_ext, version, is_current,
           section_number, last_section_number,
           payload, payload_size, source_pid);
}


//----------------------------------------------------------------------------
// Clear content.
//----------------------------------------------------------------------------

void ts::Section::clear()
{
    SuperClass::clear();
    _is_valid = false;
}


//----------------------------------------------------------------------------
// Reload from full binary content.
//----------------------------------------------------------------------------

void ts::Section::reload(const void* content, size_t content_size, PID source_pid)
{
    SuperClass::reload(content, content_size, source_pid);
    validate(CRC32::CHECK);
}

void ts::Section::reload(const ByteBlock& content, PID source_pid)
{
    SuperClass::reload(content, source_pid);
    validate(CRC32::CHECK);
}

void ts::Section::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    SuperClass::reload(content_ptr, source_pid);
    validate(CRC32::CHECK);
}

void ts::Section::reload(const void* content, size_t content_size, PID source_pid, CRC32::Validation crc_op)
{
    SuperClass::reload(content, content_size, source_pid);
    validate(crc_op);
}

void ts::Section::reload(const ByteBlock& content, PID source_pid, CRC32::Validation crc_op)
{
    SuperClass::reload(content, source_pid);
    validate(crc_op);
}

void ts::Section::reload(const ByteBlockPtr& content_ptr, PID source_pid, CRC32::Validation crc_op)
{
    SuperClass::reload(content_ptr, source_pid);
    validate(crc_op);
}


//----------------------------------------------------------------------------
// Reload short section
//----------------------------------------------------------------------------

void ts::Section::reload(TID tid, bool is_private_section, const void* payload, size_t payload_size, PID source_pid)
{
    clear();
    if (SHORT_SECTION_HEADER_SIZE + payload_size <= MAX_PRIVATE_SECTION_SIZE) {
        ByteBlockPtr data(new ByteBlock(SHORT_SECTION_HEADER_SIZE + payload_size));
        PutUInt8(data->data(), tid);
        PutUInt16(data->data() + 1, (is_private_section ? 0x4000 : 0x0000) | 0x3000 | uint16_t (payload_size & 0x0FFF));
        std::memcpy(data->data() + 3, payload, payload_size);
        reload(data, source_pid, CRC32::COMPUTE);
    }
}


//----------------------------------------------------------------------------
// Reload long section
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
    clear();
    if (section_number <= last_section_number && version <= 31 &&
        LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE <= MAX_PRIVATE_SECTION_SIZE)
    {
        ByteBlockPtr data(new ByteBlock(LONG_SECTION_HEADER_SIZE + payload_size + SECTION_CRC32_SIZE));
        PutUInt8(data->data(), tid);
        PutUInt16(data->data() + 1,
                  0x8000 | (is_private_section ? 0x4000 : 0x0000) | 0x3000 |
                  uint16_t((LONG_SECTION_HEADER_SIZE - 3 + payload_size + SECTION_CRC32_SIZE) & 0x0FFF));
        PutUInt16(data->data() + 3, tid_ext);
        PutUInt8(data->data() + 5, 0xC0 | uint8_t((version & 0x1F) << 1) | (is_current ? 0x01 : 0x00));
        PutUInt8(data->data() + 6, section_number);
        PutUInt8(data->data() + 7, last_section_number);
        std::memcpy(data->data() + 8, payload, payload_size);  // Flawfinder: ignore: memcpy()
        reload(data, source_pid, CRC32::COMPUTE);
    }
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
// Validate binary content (helper for constructors).
//----------------------------------------------------------------------------

void ts::Section::validate(CRC32::Validation crc_op)
{
    // Basic validity check using section size
    const uint8_t* const daddr = content();
    const size_t dsize = size();
    const size_t total_size = SectionSize(daddr, dsize);
    const bool is_long = StartLongSection(daddr, dsize);
    if (total_size == 0 || total_size != dsize) {
        clear();
        return;
    }

    // Extract long section header info (check section number <= last section number).
    if (is_long && (dsize < MIN_LONG_SECTION_SIZE || daddr[6] > daddr[7])) {
        clear();
        return;
    }

    // Check CRC32 if required
    if (is_long) {
        // Section size, without CRC32:
        const size_t sec_size = dsize - SECTION_CRC32_SIZE;
        switch (crc_op) {
            case CRC32::CHECK:
                if (CRC32(daddr, sec_size) != GetUInt32(daddr + sec_size)) {
                    clear();
                    return;
                }
                break;
            case CRC32::COMPUTE:
                PutUInt32(rwContent() + sec_size, CRC32(daddr, sec_size).value());
                break;
            case CRC32::IGNORE:
            default:
                break;
        }
    }

    // Passed all checks
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Assignment.
//----------------------------------------------------------------------------

ts::Section& ts::Section::operator=(const Section& sect)
{
    if (&sect != this) {
        SuperClass::operator=(sect);
        _is_valid = sect._is_valid;
    }
    return *this;
}

ts::Section& ts::Section::operator=(const Section&& sect) noexcept
{
    if (&sect != this) {
        SuperClass::operator=(std::move(sect));
        _is_valid = sect._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication.
//----------------------------------------------------------------------------

ts::Section& ts::Section::copy(const Section& sect)
{
    if (&sect != this) {
        SuperClass::copy(sect);
        _is_valid = sect._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison. Note: Invalid sections are never identical
//----------------------------------------------------------------------------

bool ts::Section::operator==(const Section& sect) const
{
    return _is_valid && sect._is_valid && SuperClass::operator==(sect);
}


//----------------------------------------------------------------------------
// This method recomputes and replaces the CRC32 of the section.
//----------------------------------------------------------------------------

void ts::Section::recomputeCRC()
{
    if (isLongSection()) {
        // Section size, without CRC32:
        const size_t sec_size = size() - SECTION_CRC32_SIZE;
        PutUInt32(rwContent() + sec_size, CRC32(content(), sec_size).value());
    }
}

//----------------------------------------------------------------------------
// Get a hash of the section content.
//----------------------------------------------------------------------------

ts::ByteBlock ts::Section::hash() const
{
    ByteBlock result;
    if (isValid()) {
        SHA1 algo;
        algo.hash(content(), size(), result);
    }
    return result;
}


//----------------------------------------------------------------------------
// Implementation of AbstractDefinedByStandards.
//----------------------------------------------------------------------------

ts::Standards ts::Section::definingStandards() const
{
    // The defining standard is taken from table id.
    return PSIRepository::Instance().getTableStandards(tableId(), sourcePID());
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

void ts::Section::setTableId(uint8_t tid, bool recompute_crc)
{
    if (_is_valid) {
        rwContent()[0] = tid;
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setTableIdExtension(uint16_t tid_ext, bool recompute_crc)
{
    if (isLongSection()) {
        PutUInt16(rwContent() + 3, tid_ext);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setVersion(uint8_t version, bool recompute_crc)
{
    if (isLongSection()) {
        rwContent()[5] = (content()[5] & 0xC1) | uint8_t((version & 0x1F) << 1);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setIsCurrent(bool is_current, bool recompute_crc)
{
    if (isLongSection()) {
        rwContent()[5] = (content()[5] & 0xFE) | (is_current ? 0x01 : 0x00);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setSectionNumber(uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        rwContent()[6] = num;
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setLastSectionNumber(uint8_t num, bool recompute_crc)
{
    if (isLongSection()) {
        rwContent()[7] = num;
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setUInt8(size_t offset, uint8_t value, bool recompute_crc)
{
    if (_is_valid && offset < payloadSize()) {
        PutUInt8(rwContent() + headerSize() + offset, value);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setUInt16(size_t offset, uint16_t value, bool recompute_crc)
{
    if (_is_valid && offset + 1 < payloadSize()) {
        PutUInt16(rwContent() + headerSize() + offset, value);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}

void ts::Section::setUInt32(size_t offset, uint32_t value, bool recompute_crc)
{
    if (_is_valid && offset + 3 < payloadSize()) {
        PutUInt32(rwContent() + headerSize() + offset, value);
        if (recompute_crc) {
            recomputeCRC();
        }
    }
}


//----------------------------------------------------------------------------
// Append binary data to the payload of the section.
//----------------------------------------------------------------------------

void ts::Section::appendPayload(const void* data, size_t dsize, bool recompute_crc)
{
    if (_is_valid && data != nullptr && dsize != 0) {
        // Update section size in header.
        PutUInt16(rwContent() + 1, (GetUInt16(content() + 1) & 0xF000) | uint16_t((size() + dsize - 3) & 0x0FFF));

        // Remove trailing CRC (now invalid) at end of long section.
        const bool is_long = isLongSection() && size() >= LONG_SECTION_HEADER_SIZE + 4;
        if (is_long) {
            rwResize(size() - 4);
        }

        // Append the data.
        rwAppend(data, dsize);

        // Restore a trailing CRC at end of long section and optionally recompute it.
        if (is_long) {
            static const uint8_t byte4[4] = {0, 0, 0, 0};
            rwAppend(byte4, sizeof(byte4));
            if (recompute_crc) {
                recomputeCRC();
            }
        }
    }
}


//----------------------------------------------------------------------------
// Truncate the payload of the section.
//----------------------------------------------------------------------------

void ts::Section::truncatePayload(size_t dsize, bool recompute_crc)
{
    const size_t previous_size = payloadSize();

    // Do something only if the payload is really truncated.
    if (_is_valid && dsize < previous_size) {

        // Size to be removed from section:
        const size_t remove = previous_size - dsize;

        // Update section size in header.
        PutUInt16(rwContent() + 1, (GetUInt16(content() + 1) & 0xF000) | uint16_t((size() - remove - 3) & 0x0FFF));

        // Truncate the section.
        rwResize(size() - remove);

        // Optionally recompute it.
        if (recompute_crc && isLongSection()) {
            recomputeCRC();
        }
    }
}


//----------------------------------------------------------------------------
// Write section on standard streams.
//----------------------------------------------------------------------------

std::ostream& ts::Section::write(std::ostream& strm, Report& report) const
{
    if (_is_valid && strm) {
        strm.write(reinterpret_cast<const char*>(content()), std::streamsize(size()));
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
    strm.read(reinterpret_cast<char*>(header), 3);
    size_t insize = size_t(strm.gcount());

    // Read rest of the section
    if (insize == 3) {
        secsize += GetUInt16(header + 1) & 0x0FFF;
        secdata = new ByteBlock(secsize);
        CheckNonNull(secdata.pointer());
        std::memcpy(secdata->data(), header, 3);  // Flawfinder: ignore: memcpy()
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
    const UString margin(indent, ' ');
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
             << UString::Format(u"* Section dump, PID 0x%X (%<d), TID %s", {sourcePID(), names::TID(duck, tid, cas, NamesFlags::BOTH_FIRST)})
             << std::endl
             << margin << "  Section size: " << size() << " bytes, header: " << (isLongSection() ? "long" : "short")
             << std::endl;
        if (isLongSection()) {
            strm << margin
                 << UString::Format(u"  TIDext: 0x%X (%<d), version: %d, index: %d, last: %d, %s",
                                    {tableIdExtension(), version(), sectionNumber(), lastSectionNumber(), (isNext() ? u"next" : u"current")})
                 << std::endl;
        }
    }

    // Display section body
    strm << UString::Dump(content(), size(), UString::HEXA | UString::ASCII | UString::OFFSET, margin.size() + 2);
    return strm;
}

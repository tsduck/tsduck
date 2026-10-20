//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsByteBlock.h"
#include "tsUString.h"
#include "tsReport.h"
#include "tsBCD.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(size_type size) :
    ByteVector(size)
{
}

ts::ByteBlock::ByteBlock(size_type size, uint8_t value) :
    ByteVector(size, value)
{
}

ts::ByteBlock::ByteBlock(const void* data, size_type size) :
    ByteVector(size)
{
    if (size > 0 && data != nullptr) {
        MemCopy(&(*this)[0], data, size);
    }
}

ts::ByteBlock::ByteBlock(const ByteBlock& bb, size_type start, size_type count) :
    ByteVector(std::min(count, bb.size() - std::min(start, bb.size())))
{
    if (size() > 0) {
        MemCopy(&(*this)[0], &bb[start], size());
    }
}

ts::ByteBlock::ByteBlock(ByteBlock&& bb) :
    ByteVector(std::move(bb))
{
}

ts::ByteBlock::ByteBlock(const char* str) :
    ByteVector(str == nullptr ? 0 : std::strlen(str))
{
    if (size() > 0) {
        MemCopy(data(), str, size());
    }
}

ts::ByteBlock::ByteBlock(std::initializer_list<uint8_t> init) :
    ByteVector(init)
{
}

//----------------------------------------------------------------------------
// Get a reference to an empty ByteBlock.
//----------------------------------------------------------------------------

const ts::ByteBlock& ts::ByteBlock::EMPTY()
{
    static const ByteBlock data;
    return data;
}

//----------------------------------------------------------------------------
// Check if a memory range is inside this object.
//----------------------------------------------------------------------------

bool ts::ByteBlock::insideThis(const void* addr, size_type length) const
{
    if (addr == nullptr || length == 0) {
        // If addr is inside this but length is zero, consider it's irrelevant or harmless.
        return false;
    }

    const auto p_start = reinterpret_cast<const uint8_t*>(addr);
    const auto p_end = p_start + length;
    const auto this_start = data();
    const auto this_end = this_start + size();

    if (p_start >= this_start && p_end <= this_end) {
        return true;
    }
    else if (p_end <= this_start || p_start >= this_end) {
        return false;
    }
    else {
        throw std::out_of_range("ByteBlock partial overlap");
    }
}

//----------------------------------------------------------------------------
// Find the first occurrence of a byte value in a byte block.
//----------------------------------------------------------------------------

ts::ByteBlock::size_type ts::ByteBlock::find(uint8_t value, size_type start, size_type end) const
{
    end = std::min(end, size());
    for (size_type i = start; i < end; ++i) {
        if ((*this)[i] == value) {
            return i;
        }
    }
    return NPOS;
}

//----------------------------------------------------------------------------
// Replace the content of a byte block.
//----------------------------------------------------------------------------

void ts::ByteBlock::copy(const void* data_, size_type size_)
{
    resize(data_ == nullptr ? 0 : size_);
    MemCopy(data(), data_, size());
}

void ts::ByteBlock::copy(const ByteBlock& bb, size_type start, size_type count)
{
    start = std::min(start, bb.size());
    count = std::min(count, bb.size() - start);
    copy(&bb[start], count);
}

//----------------------------------------------------------------------------
// Append data to a byte block.
//----------------------------------------------------------------------------

void ts::ByteBlock::append(const void* data, size_type size)
{
    if (insideThis(data, size)) {
        // Partial self-copy, use an intermediate buffer.
        const ByteBlock bb(data, size);
        MemCopy(enlarge(size), bb.data(), size);
    }
    else if (size > 0 && data != nullptr) {
        MemCopy(enlarge(size), data, size);
    }
}

void ts::ByteBlock::append(const ByteBlock& bb, size_type start, size_type count)
{
    start = std::min(start, bb.size());
    count = std::min(bb.size() - start, count);
    append(&bb[start], count);
}

//----------------------------------------------------------------------------
// Increase size by n and return pointer to new n-byte area
//----------------------------------------------------------------------------

uint8_t* ts::ByteBlock::enlarge(size_type n)
{
    const size_type oldsize = this->size();
    resize(oldsize + n);
    return data() + oldsize;
}

//----------------------------------------------------------------------------
// Append an integer in Binary Coded Decimal (BCD) representation at the end.
//----------------------------------------------------------------------------

void ts::ByteBlock::appendBCD(uint32_t value, size_t bcd_count, bool left_justified, uint8_t pad_nibble)
{
    EncodeBCD(enlarge((bcd_count + 1) / 2), bcd_count, value, left_justified, pad_nibble);
}

//----------------------------------------------------------------------------
// Append a unicode string in UTF-8 representation to a byte block.
//----------------------------------------------------------------------------

void ts::ByteBlock::appendUTF8(const UString& s)
{
    append(s.toUTF8());
}

void ts::ByteBlock::appendUTF8WithByteLength(const UString& s)
{
    const size_type oldSize = this->size();
    push_back(0);  // Placeholder for length
    appendUTF8(s);
    const size_type strSize = this->size() - oldSize - 1;
    if (strSize < 256) {
        // Update length byte.
        (*this)[oldSize] = uint8_t(strSize);
    }
    else {
        // String too long, truncate.
        resize(this->size() - strSize + 255);
        (*this)[oldSize] = 255;
    }
}

//----------------------------------------------------------------------------
// Remove 'size' elements at index 'first'.
// (the STL equivalent uses iterators, not indices).
//----------------------------------------------------------------------------

void ts::ByteBlock::erase(size_type first, size_type size)
{
    assert(first + size <= this->size());
    ByteVector::erase(begin() + first, begin() + first + size);
}

//----------------------------------------------------------------------------
// Read byte blocks from binary files.
//----------------------------------------------------------------------------

bool ts::ByteBlock::loadFromFile(const UString& file_name, size_t max_size, Report* report)
{
    clear();
    return appendFromFile(file_name, max_size, report);
}

bool ts::ByteBlock::appendFromFile(const UString& file_name, size_t max_size, Report* report)
{
    // Open the input file.
    std::ifstream strm(file_name.toUTF8().c_str(), std::ios::in | std::ios::binary);
    if (!strm.is_open()) {
        if (report != nullptr) {
            report->error(u"cannot open %s", file_name);
        }
        return false;
    }

    // Load the file content.
    append(strm, max_size);

    // Success if no error or reached EOF without error
    const bool success = !strm.fail() || strm.eof();
    strm.close();
    if (!success && report != nullptr) {
        report->error(u"error reading %s", file_name);
    }
    return success;
}

std::istream& ts::ByteBlock::read(std::istream& strm, size_t max_size)
{
    clear();
    return append(strm, max_size);
}

std::istream& ts::ByteBlock::append(std::istream& strm, size_t max_size)
{
    // Read by chunks of this size:
    constexpr size_t chunk_size = 32 * 1024;

    while (!strm.fail() && !strm.eof() && max_size > 0) {

        // Size of the next chunk to read.
        size_t read_size = std::min(max_size, chunk_size);

        // Make more space in byte block for reading a chunk.
        const size_t previous_size = size();
        resize(previous_size + read_size);

        // Read a chunk of data.
        strm.read(reinterpret_cast<char*>(data() + previous_size), std::streamsize(read_size));
        const std::streamsize gc = strm.gcount();
        read_size = gc < 0 ? 0 : std::min(size_t(gc), read_size);

        // Adjust buffer size.
        resize(previous_size + read_size);
    }

    return strm;
}

//----------------------------------------------------------------------------
// Save byte blocks to binary files.
//----------------------------------------------------------------------------

bool ts::ByteBlock::saveToFile(const UString& file_name, Report* report) const
{
    return writeToFile(file_name, std::ios::out | std::ios::binary, report);
}

bool ts::ByteBlock::appendToFile(const UString& file_name, Report* report) const
{
    return writeToFile(file_name, std::ios::out | std::ios::app | std::ios::binary, report);
}

std::ostream& ts::ByteBlock::write(std::ostream& strm) const
{
    return strm.write(reinterpret_cast<const char*>(data()), std::streamsize(size()));
}

bool ts::ByteBlock::writeToFile(const UString& file_name, std::ios::openmode mode, Report* report) const
{
    // Create the file
    std::ofstream strm(file_name.toUTF8().c_str(), mode);
    if (!strm.is_open()) {
        if (report != nullptr) {
            report->error(u"cannot create %s", file_name);
        }
        return false;
    }

    // Write the content.
    write(strm);
    const bool success = !strm.fail();
    strm.close();
    if (!success && report != nullptr) {
        report->error(u"error writing %s", file_name);
    }
    return success;
}

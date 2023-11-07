//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsByteBlock.h"
#include "tsUString.h"
#include "tsReport.h"
#include "tsBCD.h"


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(size_type size) :
    ByteVector(size)
{
}

//----------------------------------------------------------------------------
// Constructor, initialized with size bytes of specified value
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(size_type size, uint8_t value) :
    ByteVector(size, value)
{
}

//----------------------------------------------------------------------------
// Constructor from a data block
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(const void* data, size_type size) :
    ByteVector(size)
{
    if (size > 0) {
        std::memcpy(&(*this)[0], data, size);  // Flawfinder: ignore: memcpy()
    }
}

//----------------------------------------------------------------------------
// Constructor from a C string
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(const char* str) :
    ByteVector(::strlen(str))  // Flawfinder: ignore: strlen()
{
    if (size() > 0) {
        std::memcpy(data(), str, size());  // Flawfinder: ignore: memcpy()
    }
}

//----------------------------------------------------------------------------
// Constructor from an initializer list.
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(std::initializer_list<uint8_t> init) :
    ByteVector(init)
{
}

//----------------------------------------------------------------------------
// Find the first occurence of a byte value in a byte block.
//----------------------------------------------------------------------------

ts::ByteBlock::size_type ts::ByteBlock::find(uint8_t value, size_type start)
{
    for (size_type i = start; i < size(); ++i) {
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
    if (size() > 0) {
        std::memcpy(data(), data_, size());  // Flawfinder: ignore: memcpy()
    }
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

bool ts::ByteBlock::loadFromFile(const UString& fileName, size_t maxSize, Report* report)
{
    clear();
    return appendFromFile(fileName, maxSize, report);
}

bool ts::ByteBlock::appendFromFile(const UString& fileName, size_t maxSize, Report* report)
{
    // Open the input file.
    std::ifstream strm(fileName.toUTF8().c_str(), std::ios::in | std::ios::binary);
    if (!strm.is_open()) {
        if (report != nullptr) {
            report->error(u"cannot open %s", {fileName});
        }
        return false;
    }

    // Load the file content.
    append(strm, maxSize);

    // Success if no error or reached EOF without error
    const bool success = !strm.fail() || strm.eof();
    strm.close();
    if (!success && report != nullptr) {
        report->error(u"error reading %s", {fileName});
    }
    return success;
}

std::istream& ts::ByteBlock::read(std::istream& strm, size_t maxSize)
{
    clear();
    return append(strm, maxSize);
}

std::istream& ts::ByteBlock::append(std::istream& strm, size_t maxSize)
{
    // Read by chunks of this size:
    constexpr size_t chunkSize = 32 * 1024;

    while (!strm.fail() && !strm.eof() && maxSize > 0) {

        // Size of the next chunk to read.
        size_t readSize = std::min(maxSize, chunkSize);

        // Make more space in byte block for reading a chunk.
        const size_t previousSize = size();
        resize(previousSize + readSize);

        // Read a chunk of data.
        strm.read(reinterpret_cast<char*>(data() + previousSize), std::streamsize(readSize));
        const std::streamsize gc = strm.gcount();
        readSize = gc < 0 ? 0 : std::min(size_t(gc), readSize);

        // Adjust buffer size.
        resize(previousSize + readSize);
    }

    return strm;
}

//----------------------------------------------------------------------------
// Save byte blocks to binary files.
//----------------------------------------------------------------------------

bool ts::ByteBlock::saveToFile(const UString& fileName, Report* report) const
{
    return writeToFile(fileName, std::ios::out | std::ios::binary, report);
}

bool ts::ByteBlock::appendToFile(const UString& fileName, Report* report) const
{
    return writeToFile(fileName, std::ios::out | std::ios::app | std::ios::binary, report);
}

std::ostream& ts::ByteBlock::write(std::ostream& strm) const
{
    return strm.write(reinterpret_cast<const char*>(data()), std::streamsize(size()));
}

bool ts::ByteBlock::writeToFile(const UString& fileName, std::ios::openmode mode, Report* report) const
{
    // Create the file
    std::ofstream strm(fileName.toUTF8().c_str(), mode);
    if (!strm.is_open()) {
        if (report != nullptr) {
            report->error(u"cannot create %s", {fileName});
        }
        return false;
    }

    // Write the content.
    write(strm);
    const bool success = !strm.fail();
    strm.close();
    if (!success && report != nullptr) {
        report->error(u"error writing %s", {fileName});
    }
    return success;
}

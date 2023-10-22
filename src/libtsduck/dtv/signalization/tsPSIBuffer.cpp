//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsDescriptorList.h"
#include "tsSection.h"
#include "tsMJD.h"
#include "tsATSCMultipleString.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PSIBuffer::PSIBuffer(DuckContext& duck, size_t size) :
    Buffer(size),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, void* data, size_t size, bool read_only) :
    Buffer(data, size, read_only),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, const void* data, size_t size) :
    Buffer(data, size),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, const Section& section) :
    Buffer(section.payload(), section.payloadSize()),
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Serialize / deserialize a 13-bit PID value.
//----------------------------------------------------------------------------

ts::PID ts::PSIBuffer::getPID()
{
    if (readIsByteAligned()) {
        skipReservedBits(3);
    }
    if (currentReadBitOffset() % 8 == 3) {
        return getBits<PID>(13);
    }
    else {
        setReadError();
        return PID_NULL;
    }
}

bool ts::PSIBuffer::putPID(PID pid)
{
    if (writeIsByteAligned()) {
        return putUInt16(0xE000 | pid);
    }
    else if (currentWriteBitOffset() % 8 == 3) {
        return putBits(pid, 13);
    }
    else {
        setWriteError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Serialize a 3-byte language or country code.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putLanguageCode(const UString& str, bool allow_empty)
{
    // Process empty strings as zeroes when allowed.
    if (allow_empty && str.empty()) {
        putUInt24(0);
        return !writeError();
    }

    // Generate an error if the provided code is not 3 characters long or not ASCII-only.
    // All country codes are encoded in ASCII, no exception allowed.
    bool ok = str.size() == 3;
    for (size_t i = 0; ok && i < 3; ++i) {
        ok = int(str[i]) >= 0x20 && int(str[i]) <= 0x7F;
    }
    if (ok) {
        for (size_t i = 0; i < 3; ++i) {
            putUInt8(uint8_t(str[i]));
        }
        return !writeError();
    }
    else {
        setWriteError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Deserialize a 3-byte language or country code.
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getLanguageCode()
{
    UString str;
    getLanguageCode(str);
    return str;
}

bool ts::PSIBuffer::getLanguageCode(UString& str)
{
    str.clear();

    if (readError() || remainingReadBytes() < 3 || !readIsByteAligned()) {
        // No partial string read if not enough bytes are present.
        // Cannot read unaligned character codes.
        setReadError();
        return false;
    }
    else {
        // Read 3 characters. Ignore non-ASCII characters.
        for (size_t i = 0; i < 3; ++i) {
            const uint8_t c = getUInt8();
            if (c >= 0x20 && c <= 0x7F) {
                str.push_back(UChar(c));
            }
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Common code the various putString functions.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putStringCommon(const UString& str, size_t start, size_t count, EncodeMethod em, bool partial, size_t min_req_size, const Charset* charset)
{
    // Make sure we can write in the buffer and has the minimum required free size.
    if (readOnly() || writeError() || remainingWriteBytes() < min_req_size) {
        setWriteError();
        return 0;
    }

    // Adjust index and size to allowed bounds.
    start = std::min(start, str.size());
    count = std::min(count, str.size() - start);

    // Encode the string.
    uint8_t* data = currentWriteAddress();
    const size_t prev_size = remainingWriteBytes();
    size_t size = prev_size;
    const size_t nchars = (_duck.charsetOut(charset)->*em)(data, size, str, start, count);

    if (partial || nchars >= count) {
        // Some or all characters were serialized.
        // Include the serialized bytes in the written part.
        writeSeek(currentWriteByteOffset() + prev_size - size);
        return partial ? nchars : size_t(!writeError());
    }
    else {
        // Failed to serialize the whole string.
        setWriteError();
        return 0;
    }
}


//----------------------------------------------------------------------------
// Deserialize a string
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getString(size_t size, const Charset* charset)
{
    UString str;
    getString(str, size, charset);
    return str;
}

bool ts::PSIBuffer::getString(ts::UString& str, size_t size, const Charset* charset)
{
    // NPOS means exact size of the buffer.
    if (size == NPOS) {
        size = remainingReadBytes();
    }
    if (readError() || size > remainingReadBytes()) {
        str.clear();
        setReadError();
        return false;
    }

    // Decode characters. Ignore decoding errors since it could be simply an unsupported character.
    _duck.charsetIn(charset)->decode(str, currentReadAddress(), size);

    // Include the deserialized bytes in the read part.
    readSeek(currentReadByteOffset() + size);
    return true;
}


//----------------------------------------------------------------------------
// Deserialize a string with byte length.
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getStringWithByteLength(const Charset* charset)
{
    UString str;
    getStringWithByteLength(str, charset);
    return str;
}

bool ts::PSIBuffer::getStringWithByteLength(ts::UString& str, const Charset* charset)
{
    const size_t size = getUInt8();
    return getString(str, std::min<size_t>(size, remainingReadBytes()), charset);
}


//----------------------------------------------------------------------------
// Serialize and deserialize dates and times.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putMJD(const Time& time, size_t mjd_size)
{
    if (readOnly() || writeError() || !writeIsByteAligned() || remainingWriteBytes() < mjd_size || !EncodeMJD(time, currentWriteAddress(), mjd_size)) {
        // Write is not byte-aligned or there is not enough room or encoding error.
        setWriteError();
        return false;
    }
    else {
        // Successfully serialized, move write pointer.
        writeSeek(currentWriteByteOffset() + mjd_size);
        return true;
    }
}

ts::Time ts::PSIBuffer::getMJD(size_t mjd_size)
{
    // Invalid MJD decoding: We filter invalid mjd_size as an error.
    // But we accept invalid MJD values (returns Unix Epoch) because
    // too many EIT's have invalid dates in the field.
    Time result;
    if (readError() || !readIsByteAligned() || remainingReadBytes() < mjd_size || mjd_size < MJD_MIN_SIZE || mjd_size > MJD_SIZE) {
        setReadError();
        return Time::Epoch;
    }
    else {
        DecodeMJD(currentReadAddress(), mjd_size, result);
        skipBytes(mjd_size);
        return result;
    }
}


//----------------------------------------------------------------------------
// Serialize and deserialize durations in BCD digits.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putMinutesBCD(SubSecond duration)
{
    return putBCD(std::abs(duration) / 60, 2) &&
           putBCD(std::abs(duration) % 60, 2);
}

bool ts::PSIBuffer::putSecondsBCD(Second duration)
{
    return putBCD(std::abs(duration) / 3600, 2) &&
           putBCD((std::abs(duration) / 60) % 60, 2) &&
           putBCD(std::abs(duration) % 60, 2);
}

ts::SubSecond ts::PSIBuffer::getMinutesBCD()
{
    const SubSecond hours = getBCD<SubSecond>(2);
    const SubSecond minutes = getBCD<SubSecond>(2);
    return (hours * 60) + minutes;
}

ts::Second ts::PSIBuffer::getSecondsBCD()
{
    const SubSecond hours = getBCD<SubSecond>(2);
    const SubSecond minutes = getBCD<SubSecond>(2);
    const SubSecond seconds = getBCD<SubSecond>(2);
    return (hours * 3600) + (minutes * 60) + seconds;
}


//----------------------------------------------------------------------------
// Serialize and deserialize integer values in "vluimsbf5" format.
// Mind the Rabbit of Caerbannog.
//----------------------------------------------------------------------------

uint64_t ts::PSIBuffer::getVluimsbf5()
{
    // Get the number of 4-Bit fields.
    size_t n = 1;
    while (!readError() && getBit() == 1) {
        n++;
    }

    // Get the integer value.
    return getBits<uint64_t>(4 * n);
}

bool ts::PSIBuffer::putVluimsbf5(uint64_t value)
{
    // Compute the required number of 4-bit fields. The maximum value is 16 (full 64-bit unsigned int).
    size_t n = 1;
    uint64_t tmp = value;
    while (tmp > 0x0F) {
        n++;
        tmp = tmp >> 4;
    }

    // Serialize n. Then serialize the value.
    return putBits(0xFFFFFFFF, n - 1) && putBit(0) && putBits(value, 4 * n);
}


//----------------------------------------------------------------------------
// Put (serialize) a complete descriptor list.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putDescriptorList(const DescriptorList& descs, size_t start, size_t count)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);

    if (readOnly() || writeError() || !writeIsByteAligned() || descs.binarySize(start, count) > remainingWriteBytes()) {
        // Write is not byte-aligned or there is not enough room to serialize the descriptors.
        setWriteError();
        return false;
    }
    else {
        // Write all descriptors (they should fit).
        const size_t next = putPartialDescriptorList(descs, start, count);
        assert(next == start + count);
        return true;
    }
}


//----------------------------------------------------------------------------
// Put (serialize) as many descriptors as possible from a descriptor list.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putPartialDescriptorList(const DescriptorList& descs, size_t start, size_t count)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);
    const size_t last = start + count;

    // Write error if not byte-aligned.
    if (readOnly() || writeError() || !writeIsByteAligned()) {
        setWriteError();
        return start;
    }

    // Serialize as many descriptors as we can.
    while (start < last && descs[start]->size() <= remainingWriteBytes()) {
        const size_t written = putBytes(descs[start]->content(), descs[start]->size());
        assert(written == descs[start]->size());
        start++;
    }

    return start;
}


//----------------------------------------------------------------------------
// Put (serialize) a complete descriptor list with a 2-byte length field.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putDescriptorListWithLength(const DescriptorList& descs, size_t start, size_t count, size_t length_bits)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);

    if (2 + descs.binarySize(start, count) > remainingWriteBytes()) {
        // Not enough room to serialize the descriptors.
        setWriteError();
        return false;
    }
    else {
        // Write all descriptors (they should fit unless there is an alignment error).
        return putPartialDescriptorListWithLength(descs, start, count, length_bits) == start + count;
    }
}


//----------------------------------------------------------------------------
// Put (serialize) as many descriptors as possible with a 2-byte length field.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putPartialDescriptorListWithLength(const DescriptorList& descs, size_t start, size_t count, size_t length_bits)
{
    // Normalize start.
    start = std::min(start, descs.size());

    // Filter incorrect length or length alignment.
    if (readOnly() || writeError() || remainingWriteBytes() < 2 || length_bits == 0 || length_bits > 16 || (!writeIsByteAligned() && currentWriteBitOffset() % 8 != 16 - length_bits)) {
        setWriteError();
        return start;
    }

    // Write stuffing bits if byte aligned.
    if (writeIsByteAligned()) {
        putBits(0xFFFF, 16 - length_bits);
    }

    // Save state where the length will be written later.
    pushWriteSequenceWithLeadingLength(length_bits);

    // Serialize as many descriptors as we can. Compute written size.
    start = putPartialDescriptorList(descs, start, count);

    // Update the length field.
    popState();

    return start;
}


//----------------------------------------------------------------------------
// Get (deserialize) a descriptor list.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getDescriptorList(DescriptorList& descs, size_t length)
{
    // Normalize and check length.
    if (length == NPOS) {
        length = remainingReadBytes();
    }
    if (!readIsByteAligned() || length > remainingReadBytes()) {
        setReadError();
        return false;
    }

    // Read descriptors.
    const bool ok = descs.add(currentReadAddress(), length);
    skipBytes(length);

    if (!ok) {
        setReadError();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get (deserialize) a descriptor list with a 2-byte length field.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getDescriptorListWithLength(DescriptorList& descs, size_t length_bits)
{
    // Read the length field.
    const size_t length = getUnalignedLength(length_bits);
    bool ok = !readError();

    // Read descriptors.
    if (ok) {
        ok = descs.add(currentReadAddress(), length);
        skipBytes(length);
    }

    if (!ok) {
        setReadError();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a 2-byte integer field, typically a length before a descriptor list.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::getUnalignedLength(size_t length_bits)
{
    if (readError() || remainingReadBytes() < 2 || length_bits == 0 || length_bits > 16 || (!readIsByteAligned() && (currentReadBitOffset() + length_bits) % 8 != 0)) {
        setReadError();
        return 0;
    }
    if (readIsByteAligned()) {
        skipReservedBits(16 - length_bits);
    }
    const size_t length = getBits<size_t>(length_bits);
    const size_t actual_length = std::min(length, remainingReadBytes());
    assert(readIsByteAligned());
    if (length > actual_length) {
        setReadError();
    }
    return actual_length;
}


//----------------------------------------------------------------------------
// Get an ATSC multiple_string_structure.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getMultipleString(ATSCMultipleString& mss, size_t mss_size, bool ignore_empty)
{
    mss.clear();

    // Must start on a byte boundary.
    if (readError() || !readIsByteAligned()) {
        setReadError();
        return false;
    }

    // These pointers will be updated by mss.deserialize().
    const uint8_t* data = currentReadAddress();
    size_t size = remainingReadBytes();

    // Make sure mss_size is actually used if lower than NPOS but larger than buffer size.
    if (mss_size != NPOS && mss_size > size) {
        mss_size = size;
    }

    // Deserialize the multiple string structure.
    if (mss.deserialize(_duck, data, size, mss_size, ignore_empty)) {
        assert(size <= remainingReadBytes());
        skipBytes(remainingReadBytes() - size);
        return true;
    }
    else {
        setReadError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an ATSC multiple_string_structure with a leading byte length.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getMultipleStringWithLength(ATSCMultipleString& mss, size_t length_bytes)
{
    mss.clear();

    // Must start on a byte boundary.
    if (readError() || !readIsByteAligned()) {
        setReadError();
        return false;
    }

    // These pointers will be updated by mss.deserialize().
    const uint8_t* data = currentReadAddress();
    size_t size = remainingReadBytes();

    // Deserialize the multiple string structure.
    if (mss.lengthDeserialize(_duck, data, size, length_bytes)) {
        assert(size <= remainingReadBytes());
        skipBytes(remainingReadBytes() - size);
        return true;
    }
    else {
        setReadError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Put an ATSC multiple_string_structure.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putMultipleString(const ATSCMultipleString& mss, size_t max_size, bool ignore_empty)
{
    // Must start on a byte boundary.
    if (readOnly() || writeError() || !writeIsByteAligned()) {
        setReadError();
        return false;
    }

    // These pointers will be updated by mss.deserialize().
    uint8_t* data = currentWriteAddress();
    size_t size = remainingWriteBytes();

    // Serialize the structure.
    size_t count = mss.serialize(_duck, data, size, max_size, ignore_empty);

    // Successfully serialized, move write pointer.
    assert(count <= remainingWriteBytes());
    writeSeek(currentWriteByteOffset() + count);
    return true;
}


//----------------------------------------------------------------------------
// Put an ATSC multiple_string_structure with a leading byte length.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putMultipleStringWithLength(const ATSCMultipleString& mss, size_t length_bytes)
{
    // Must start on a byte boundary.
    if (readOnly() || writeError() || !writeIsByteAligned()) {
        setReadError();
        return false;
    }

    // These pointers will be updated by mss.deserialize().
    uint8_t* data = currentWriteAddress();
    size_t size = remainingWriteBytes();

    // Serialize the structure.
    mss.lengthSerialize(_duck, data, size, length_bytes);

    // Successfully serialized, move write pointer.
    assert(size <= remainingWriteBytes());
    writeSeek(currentWriteByteOffset() + remainingWriteBytes() - size);
    return true;
}

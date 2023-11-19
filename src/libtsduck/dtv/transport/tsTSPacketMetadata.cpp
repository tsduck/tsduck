//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPacketMetadata.h"
#include <cstddef>


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSPacketMetadata::TSPacketMetadata() :
    _input_time(INVALID_PCR),
    _labels(),
    _time_source(TimeSource::UNDEFINED),
    _flush(false),
    _bitrate_changed(false),
    _input_stuffing(false),
    _nullified(false),
    _pad1(0),
    _pad2(0)
{
}


//----------------------------------------------------------------------------
// Reset the content of this instance.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::reset()
{
    _input_time = INVALID_PCR;
    _labels.reset();
    _flush = false;
    _bitrate_changed = false;
    _input_stuffing = false;
    _nullified = false;
}


//----------------------------------------------------------------------------
// Get the list of labels as a string, typically for debug messages.
//----------------------------------------------------------------------------

ts::UString ts::TSPacketMetadata::labelsString(const UString& separator, const UString& none) const
{
    if (_labels.none()) {
        return none;
    }
    else {
        UString str;
        for (size_t lab = 0; lab < _labels.size(); ++lab) {
            if (_labels.test(lab)) {
                if (!str.empty()) {
                    str.append(separator);
                }
                str.append(UString::Decimal(lab));
            }
        }
        return str;
    }
}


//----------------------------------------------------------------------------
// Input time stamp operations
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::setInputTimeStamp(uint64_t time_stamp, uint64_t ticks_per_second, TimeSource source)
{
    _time_source = source;

    if (ticks_per_second == 0) {
        // Clear the time stamp.
        _input_time = INVALID_PCR;
    }
    else {
        // Convert into PCR units only when needed.
        if (ticks_per_second != SYSTEM_CLOCK_FREQ) {
            // Generic conversion: (time_stamp / ticks_per_second) * SYSTEM_CLOCK_FREQ;
            // Try to avoid losing intermediate accuracy and avoid intermediate overflow at the same time.
            const uint64_t intermediate = time_stamp * SYSTEM_CLOCK_FREQ;
            if (intermediate >= time_stamp) {
                // No intermediate overflow. No accuracy is lost.
                time_stamp = intermediate / ticks_per_second;
            }
            else {
                // Intermediate overflow. Do it the opposite way, possibly loosing intermediate accuracy.
                // But, because there was an overflow, the time_stamp value but be already very large,
                // reducing the impact of intermediate accuracy loss.
                time_stamp = (time_stamp / ticks_per_second) * SYSTEM_CLOCK_FREQ;
            }
        }
        // Make sure we remain in the usual PCR range.
        // This can create an issue if the input value wraps up at 2^64.
        // In which case, the PCR value will warp at another value than PCR_SCALE.
        _input_time = time_stamp % PCR_SCALE;
    }
}

void ts::TSPacketMetadata::clearInputTimeStamp()
{
    _input_time = INVALID_PCR;
    _time_source = TimeSource::UNDEFINED;
}

ts::UString ts::TSPacketMetadata::inputTimeStampString(const UString& none) const
{
    return _input_time == INVALID_PCR ? none : UString::Decimal(_input_time);
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::serialize(ByteBlock& bin) const
{
    bin.resize(SERIALIZATION_SIZE);
    serialize(bin.data(), bin.size());
}

size_t ts::TSPacketMetadata::serialize(void* bin, size_t size) const
{
    if (size < SERIALIZATION_SIZE) {
        Zero(bin, size);
        return 0; // too short
    }
    else {
        uint8_t* data = reinterpret_cast<uint8_t*>(bin);
        data[0] = SERIALIZATION_MAGIC;
        PutUInt64(data + 1, _input_time);
        PutUInt32(data + 9, _labels.toInt());
        data[13] = (_input_stuffing ? 0x80 : 0x00) | (_nullified ? 0x40 : 0x00) | (static_cast<uint8_t>(_time_source) & 0x0F);
        return SERIALIZATION_SIZE;
    }
}

bool ts::TSPacketMetadata::deserialize(const void* bin, size_t size)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(bin);

    // We need a valid binary structure.
    if (data == nullptr || size == 0 || data[0] != SERIALIZATION_MAGIC) {
        size = 0;
    }

    _input_time = size >= 9 ? GetUInt64(data + 1) : INVALID_PCR;
    if (size >= 13) {
        _labels = TSPacketLabelSet(GetUInt32(data + 9));
    }
    else {
        _labels.reset();
    }
    _flush = false;
    _bitrate_changed = false;
    _input_stuffing = size > 13 && (data[13] & 0x80) != 0;
    _nullified = size > 13 && (data[13] & 0x40) != 0;
    _time_source = static_cast<TimeSource>(data[13] & 0x0F);

    return size >= 14;
}


//----------------------------------------------------------------------------
// Static method to copy or reset contiguous TS packet metadata.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::Copy(TSPacketMetadata* dest, const TSPacketMetadata* source, size_t count)
{
    assert(dest != nullptr);
    assert(source != nullptr);
    for (size_t i = 0; i < count; ++i) {
        dest[i] = source[i];
    }
}

void ts::TSPacketMetadata::Reset(TSPacketMetadata* dest, size_t count)
{
    assert(dest != nullptr);
    for (size_t i = 0; i < count; ++i) {
        dest[i].reset();
    }
}


//----------------------------------------------------------------------------
// Display the structure layout of the data structure (for debug only).
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::DisplayLayout(std::ostream& out, const char* prefix)
{
    if (prefix == nullptr) {
        prefix = "";
    }
    ts::TSPacketMetadata var;
    out << prefix << "sizeof(TSPacketMetadata): " << sizeof(TSPacketMetadata) << " bytes" << std::endl
        << prefix << "sizeof(var): " << sizeof(var) << " bytes" << std::endl
        << prefix << "_time_source: offset: " << offsetof(TSPacketMetadata, _time_source) << " bytes, size: " << sizeof(var._time_source) << " bytes" << std::endl
        << prefix << "_labels: offset: " << offsetof(TSPacketMetadata, _labels) << " bytes, size: " << sizeof(var._labels) << " bytes" << std::endl
        << prefix << "_input_time: offset: " << offsetof(TSPacketMetadata, _input_time) << " bytes, size: " << sizeof(var._input_time) << " bytes" << std::endl;
}

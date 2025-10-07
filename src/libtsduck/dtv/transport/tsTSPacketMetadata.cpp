//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPacketMetadata.h"


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
    _datagram(false),
    _pad1(0),
    _pad2(0),
    _aux_data_size(0)
{
}


//----------------------------------------------------------------------------
// Reset the content of this instance.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::reset()
{
    _input_time = INVALID_PCR;
    _time_source = TimeSource::UNDEFINED;
    _labels.reset();
    _flush = false;
    _bitrate_changed = false;
    _input_stuffing = false;
    _nullified = false;
    _datagram = false;
    _aux_data_size = 0;
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
// Auxiliary data operations.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::setAuxData(const void* data, size_t size)
{
    _aux_data_size = data == nullptr ? 0 : uint8_t(std::min<size_t>(size, AUX_DATA_MAX_SIZE));
    MemCopy(_aux_data, data, _aux_data_size);
}

size_t ts::TSPacketMetadata::getAuxData(void* data, size_t max_size) const
{
    const size_t size = data == nullptr ? 0 : std::min<size_t>(max_size, _aux_data_size);
    MemCopy(data, _aux_data, size);
    return size;
}

void ts::TSPacketMetadata::getAuxData(void* data, size_t max_size, uint8_t pad) const
{
    if (data != nullptr) {
        const size_t size = std::min<size_t>(max_size, _aux_data_size);
        MemCopy(data, _aux_data, size);
        MemSet(reinterpret_cast<uint8_t*>(data) + size, pad, max_size - size);
    }
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
        MemZero(bin, size);
        return 0; // too short
    }
    else {
        uint8_t* data = reinterpret_cast<uint8_t*>(bin);
        data[0] = SERIALIZATION_MAGIC;
        PutUInt64(data + 1, _input_time);
        PutUInt32(data + 9, _labels.toInt());
        data[13] = (_input_stuffing ? 0x80 : 0x00) | (_nullified ? 0x40 : 0x00) | (_datagram ? 0x20 : 0x00) | (static_cast<uint8_t>(_time_source) & 0x0F);
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
    _datagram = size > 13 && (data[13] & 0x20) != 0;
    _time_source = size > 13 ? static_cast<TimeSource>(data[13] & 0x0F) : TimeSource::UNDEFINED;

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
        << prefix << "_input_time: offset: " << offsetof(TSPacketMetadata, _input_time) << " bytes, size: " << sizeof(var._input_time) << " bytes" << std::endl
        << prefix << "_aux_data_size: offset: " << offsetof(TSPacketMetadata, _aux_data_size) << " bytes, size: " << sizeof(var._aux_data_size) << " bytes" << std::endl
        << prefix << "_aux_data: offset: " << offsetof(TSPacketMetadata, _aux_data) << " bytes, size: " << sizeof(var._aux_data) << " bytes" << std::endl;
}

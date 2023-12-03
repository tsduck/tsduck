//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDemuxedData.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::DemuxedData::DemuxedData(PID source_pid) :
    _source_pid(source_pid)
{
}

ts::DemuxedData::DemuxedData(const DemuxedData& pp, ShareMode mode) :
    _source_pid(pp._source_pid),
    _first_pkt(pp._first_pkt),
    _last_pkt(pp._last_pkt)
{
    switch (mode) {
        case ShareMode::SHARE:
            _data = pp._data;
            break;
        case ShareMode::COPY:
            _data = new ByteBlock(*pp._data);
            break;
        default:
            // should not get there
            assert(false);
    }
}

ts::DemuxedData::DemuxedData(DemuxedData&& pp) noexcept :
    _source_pid(pp._source_pid),
    _first_pkt(pp._first_pkt),
    _last_pkt(pp._last_pkt),
    _data(std::move(pp._data))
{
}

ts::DemuxedData::DemuxedData(const void* content, size_t content_size, PID source_pid) :
    _source_pid(source_pid),
    _data(new ByteBlock(content, content_size))
{
}

ts::DemuxedData::DemuxedData(const ByteBlock& content, PID source_pid) :
    _source_pid(source_pid),
    _data(new ByteBlock(content))
{
}

ts::DemuxedData::DemuxedData(const ByteBlockPtr& content_ptr, PID source_pid) :
    _source_pid(source_pid),
    _data(content_ptr)
{
}

ts::DemuxedData::~DemuxedData()
{
}


//----------------------------------------------------------------------------
// Clear packet content.
//----------------------------------------------------------------------------

void ts::DemuxedData::clear()
{
    _first_pkt = 0;
    _last_pkt = 0;
    _data.clear();
}


//----------------------------------------------------------------------------
// Reload from full binary content.
//----------------------------------------------------------------------------

void ts::DemuxedData::reload(const void* content, size_t content_size, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    _data = new ByteBlock(content, content_size);
}

void ts::DemuxedData::reload(const ByteBlock& content, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    _data = new ByteBlock(content);
}

void ts::DemuxedData::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    _data = content_ptr;
}


//----------------------------------------------------------------------------
// Assignment and duplication.
//----------------------------------------------------------------------------

ts::DemuxedData& ts::DemuxedData::operator=(const DemuxedData& pp)
{
    if (&pp != this) {
        _source_pid = pp._source_pid;
        _first_pkt = pp._first_pkt;
        _last_pkt = pp._last_pkt;
        _data = pp._data;
    }
    return *this;
}

ts::DemuxedData& ts::DemuxedData::operator=(DemuxedData&& pp) noexcept
{
    if (&pp != this) {
        _source_pid = pp._source_pid;
        _first_pkt = pp._first_pkt;
        _last_pkt = pp._last_pkt;
        _data = std::move(pp._data);
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the packet
// is duplicated.
//----------------------------------------------------------------------------

ts::DemuxedData& ts::DemuxedData::copy(const DemuxedData& pp)
{
    _first_pkt = pp._first_pkt;
    _last_pkt = pp._last_pkt;
    _data = pp._data.isNull() ? nullptr : new ByteBlock(*pp._data);
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
//----------------------------------------------------------------------------

bool ts::DemuxedData::operator==(const DemuxedData& pp) const
{
    return !_data.isNull() && !pp._data.isNull() && (_data == pp._data || *_data == *pp._data);
}


//----------------------------------------------------------------------------
// Access to the full binary content of the data.
//----------------------------------------------------------------------------

const uint8_t* ts::DemuxedData::content() const
{
    return _data.isNull() ? nullptr : _data->data();
}

size_t ts::DemuxedData::size() const
{
    // Virtual method, typically overridden by subclasses.
    return _data.isNull() ? 0 : _data->size();
}

size_t ts::DemuxedData::rawDataSize() const
{
    // Non-virtual method, always the same result.
    return _data.isNull() ? 0 : _data->size();
}

void ts::DemuxedData::rwResize(size_t s)
{
    if (_data.isNull()) {
        _data = new ByteBlock(s);
    }
    else {
        _data->resize(s);
    }
}

void ts::DemuxedData::rwAppend(const void* data, size_t dsize)
{
    if (_data.isNull()) {
        _data = new ByteBlock(data, dsize);
    }
    else {
        _data->append(data, dsize);
    }
}


//----------------------------------------------------------------------------
// Check if the start of the data matches a given pattern.
//----------------------------------------------------------------------------

bool ts::DemuxedData::matchContent(const ByteBlock& pattern, const ByteBlock& mask) const
{
    // Must be at least the same size.
    if (_data.isNull() || _data->size() < pattern.size()) {
        return false;
    }
    for (size_t i = 0; i < pattern.size(); ++i) {
        const uint8_t m = i < mask.size() ? mask[i] : 0xFF;
        if (((*_data)[i] & m) != (pattern[i] & m)) {
            return false;
        }
    }
    return true;
}

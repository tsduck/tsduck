//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDemuxedData.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::DemuxedData::DemuxedData(const DemuxedData& other, ShareMode mode) :
    SuperClass(other, mode),
    _source_pid(other._source_pid),
    _first_pkt(other._first_pkt),
    _last_pkt(other._last_pkt),
    _attribute(other._attribute)
{
}

ts::DemuxedData::DemuxedData(DemuxedData&& other) noexcept :
    SuperClass(std::move(other)),
    _source_pid(other._source_pid),
    _first_pkt(other._first_pkt),
    _last_pkt(other._last_pkt),
    _attribute(std::move(other._attribute))
{
}

ts::DemuxedData::DemuxedData(const void* content, size_t content_size, PID source_pid) :
    SuperClass(content, content_size),
    _source_pid(source_pid)
{
}

ts::DemuxedData::DemuxedData(const ByteBlock& content, PID source_pid) :
    SuperClass(content),
    _source_pid(source_pid)
{
}

ts::DemuxedData::DemuxedData(const ByteBlockPtr& content_ptr, PID source_pid) :
    SuperClass(content_ptr, ShareMode::SHARE),
    _source_pid(source_pid)
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
    SuperClass::clear();
    _first_pkt = 0;
    _last_pkt = 0;
    _attribute.clear();
}


//----------------------------------------------------------------------------
// Reload from full binary content.
//----------------------------------------------------------------------------

void ts::DemuxedData::reload(const void* content, size_t content_size, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    SuperClass::reload(content, content_size);
}

void ts::DemuxedData::reload(const ByteBlock& content, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    SuperClass::reload(content);
}

void ts::DemuxedData::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    _source_pid = source_pid;
    _first_pkt = _last_pkt = 0;
    SuperClass::reload(content_ptr);
}


//----------------------------------------------------------------------------
// Assignment and duplication.
//----------------------------------------------------------------------------

ts::DemuxedData& ts::DemuxedData::operator=(const DemuxedData& other)
{
    if (&other != this) {
        _source_pid = other._source_pid;
        _first_pkt = other._first_pkt;
        _last_pkt = other._last_pkt;
        _attribute = other._attribute;
        SuperClass::operator=(other);
    }
    return *this;
}

ts::DemuxedData& ts::DemuxedData::operator=(DemuxedData&& other) noexcept
{
    if (&other != this) {
        _source_pid = other._source_pid;
        _first_pkt = other._first_pkt;
        _last_pkt = other._last_pkt;
        _attribute = std::move(other._attribute);
        SuperClass::operator=(std::move(other));
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the packet
// is duplicated.
//----------------------------------------------------------------------------

ts::DemuxedData& ts::DemuxedData::copy(const DemuxedData& other)
{
    if (&other != this) {
        _source_pid = other._source_pid;
        _first_pkt = other._first_pkt;
        _last_pkt = other._last_pkt;
        _attribute = other._attribute;
        SuperClass::copy(other);
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
//----------------------------------------------------------------------------

bool ts::DemuxedData::operator==(const DemuxedData& other) const
{
    // Don't include attributes in the comparison, they are not "part" of the demuxed object.
    return SuperClass::operator==(other);
}

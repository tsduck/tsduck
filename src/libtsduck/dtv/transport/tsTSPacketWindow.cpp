//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPacketWindow.h"


//----------------------------------------------------------------------------
// Clear the content of the packet window.
//----------------------------------------------------------------------------

void ts::TSPacketWindow::clear()
{
    _size = 0;
    _nullify_count = 0;
    _drop_count = 0;
    _last_range_index = 0;
    _ranges.clear();
}


//----------------------------------------------------------------------------
// Add the address of a range of packets and their metadata inside the window.
//----------------------------------------------------------------------------

void ts::TSPacketWindow::addPacketsReference(TSPacket* pkt, TSPacketMetadata* mdata, size_t count)
{
    assert(pkt != nullptr);
    assert(mdata != nullptr);

    // Enlarge the last range if the next packets are contiguous.
    if (!_ranges.empty()) {
        PacketRange& last(_ranges.back());
        if (pkt == last.packets + last.count && mdata == last.metadata + last.count) {
            last.count += count;
            _size += count;
            return;
        }
    }

    // Add a new range.
    _ranges.push_back({pkt, mdata, _size, count});
    _size += count;
}


//----------------------------------------------------------------------------
// Get the address of a packet or metadata inside the window.
//----------------------------------------------------------------------------

ts::TSPacket* ts::TSPacketWindow::packet(size_t index) const
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    return getInternal(index, pkt, mdata) ? pkt : nullptr;
}


ts::TSPacketMetadata* ts::TSPacketWindow::metadata(size_t index) const
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    return getInternal(index, pkt, mdata) ? mdata : nullptr;
}

bool ts::TSPacketWindow::isNullPacket(size_t index) const
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    return getInternal(index, pkt, mdata) && pkt != nullptr && pkt->getPID() == PID_NULL;
}

bool ts::TSPacketWindow::get(size_t index, TSPacket*& pkt, TSPacketMetadata*& mdata) const
{
    if (getInternal(index, pkt, mdata)) {
        return true;
    }
    else {
        pkt = nullptr;
        mdata = nullptr;
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the physical index of a packet inside a buffer.
//----------------------------------------------------------------------------

size_t ts::TSPacketWindow::packetIndexInBuffer(size_t index, const TSPacket* buffer, size_t buffer_size) const
{
    TSPacket* pkt_addr = nullptr;
    TSPacketMetadata* mdata = nullptr;
    getInternal(index, pkt_addr, mdata);

    const size_t pkt = size_t(pkt_addr);
    const size_t base = size_t(buffer);
    if (base == 0 || pkt < base) {
        // Before buffer base (including pkt_addr == nullptr).
        return NPOS;
    }
    const size_t offset = pkt - base;
    if (offset % PKT_SIZE != 0) {
        // Misaligned packet, not really part of the buffer.
        return NPOS;
    }
    const size_t buf_index = offset / PKT_SIZE;
    return buf_index < buffer_size ? buf_index : NPOS;
}


//----------------------------------------------------------------------------
// Same as public get() but returns non-null addresses for dropped packets.
//----------------------------------------------------------------------------

bool ts::TSPacketWindow::getInternal(size_t index, TSPacket*& pkt, TSPacketMetadata*& mdata) const
{
    if (index < _size) {
        // Try to reuse last range index for faster sequential index (either ascending or descending).
        // Special case for restart of sequential access.
        size_t ri = 0;
        if (index > 0) {
            ri = _last_range_index;
            assert(ri < _ranges.size());
            while (index < _ranges[ri].first) {
                assert(ri > 0);
                --ri;
            }
            while (index >= _ranges[ri].first + _ranges[ri].count) {
                ++ri;
                assert(ri < _ranges.size());
            }
        }
        // Found the right range.
        _last_range_index = ri;
        const PacketRange& ipr(_ranges[ri]);
        pkt = ipr.packets + (index - ipr.first);
        mdata = ipr.metadata + (index - ipr.first);
        // Check that the packet was not "dropped".
        return pkt->b[0] == SYNC_BYTE;
    }

    // Not found
    pkt = nullptr;
    mdata = nullptr;
    return false;
}


//----------------------------------------------------------------------------
// Nullify the packet at the corresponding index.
//----------------------------------------------------------------------------

void ts::TSPacketWindow::nullify(size_t index)
{
    TSPacket* const pkt = packet(index);
    if (pkt != nullptr && pkt->getPID() != PID_NULL) {
        // Count nullified packets once only.
        _nullify_count++;
        *pkt = NullPacket;
    }
}


//----------------------------------------------------------------------------
// Drop the packet at the corresponding index.
//----------------------------------------------------------------------------

void ts::TSPacketWindow::drop(size_t index)
{
    TSPacket* const pkt = packet(index);
    if (pkt != nullptr) {
        _drop_count++;
        pkt->b[0] = 0;
    }
}

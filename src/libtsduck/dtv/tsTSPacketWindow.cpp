//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsTSPacketWindow.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSPacketWindow::TSPacketWindow(const ts::TSPacketWindow::PacketRangeVector& ranges) :
    _size(0),
    _nullify_count(0),
    _drop_count(0),
    _last_range_index(0),
    _ranges(ranges.size())
{
    for (size_t i = 0; i < _ranges.size(); ++i) {
        assert(ranges[i].packets != nullptr);
        assert(ranges[i].metadata != nullptr);
        _ranges[i].packets = ranges[i].packets;
        _ranges[i].metadata = ranges[i].metadata;
        _ranges[i].count = ranges[i].count;
        _ranges[i].first = _size;
        _size += _ranges[i].count;
    }
}


//----------------------------------------------------------------------------
// Get the address of a packet or metadata inside the window.
//----------------------------------------------------------------------------

ts::TSPacket* ts::TSPacketWindow::packet(size_t index) const
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    get(index, pkt, mdata);
    return pkt;
}


ts::TSPacketMetadata* ts::TSPacketWindow::metadata(size_t index) const
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    get(index, pkt, mdata);
    return mdata;
}

bool ts::TSPacketWindow::get(size_t index, TSPacket*& pkt, TSPacketMetadata*& mdata) const
{
    pkt = nullptr;
    mdata = nullptr;
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
        const InternalPacketRange& ipr(_ranges[ri]);
        TSPacket* p = ipr.packets + (index - ipr.first);
        // Check that the packet was not "dropped".
        if (p->b[0] == SYNC_BYTE) {
            pkt = p;
            mdata = ipr.metadata + (index - ipr.first);
            return true;
        }
    }
    return false; // not found
}


//----------------------------------------------------------------------------
// Nullify the packet at the corresponding index.
//----------------------------------------------------------------------------

void ts::TSPacketWindow::nullify(size_t index)
{
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    if (get(index, pkt, mdata) && pkt->getPID() != PID_NULL) {
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
    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    if (get(index, pkt, mdata) && pkt->b[0] != 0) {
        // Count dropped packets once only.
        _drop_count++;
        pkt->b[0] = 0;
    }
}

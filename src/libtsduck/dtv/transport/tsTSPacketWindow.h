//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!
//!  @file
//!  A view over a window of a buffer of TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"

namespace ts {
    //!
    //! A view over a window of a buffer of TS packets.
    //! @ingroup mpeg
    //!
    //! An instance of this class encapsulates a view over a logical buffer of TS packets.
    //! The logical view is a set of N packets with associated metadata. The physical
    //! implementation of the packets can be non-contiguous (scattered buffer).
    //!
    //! The typical usage of this class is a logically contiguous view of a window over
    //! a circular buffer and/or a buffer with unused entries. This is the case of the
    //! TSProcessor global buffer.
    //!
    class TSDUCKDLL TSPacketWindow
    {
        TS_NOCOPY(TSPacketWindow);
    public:
        //!
        //! Constructor.
        //!
        TSPacketWindow();

        //!
        //! Clear the content of the packet window.
        //!
        void clear();

        //!
        //! Add the address of a range of packets and their metadata inside the window.
        //! @param [in] packet The address of the first packet.
        //! @param [in] metadata The address of the first corresponding packet metadata.
        //! @param [in] count Number of contiguous packets and metadata.
        //!
        void addPacketsReference(TSPacket* packet, TSPacketMetadata* metadata, size_t count);

        //!
        //! Get the number of packets in this window.
        //! @return The number of packets in this window.
        //!
        size_t size() const { return _size; }

        //!
        //! Get the address of a packet inside the window.
        //! @param [in] index Index of the packet inside the window, from 0 to size()-1.
        //! @return The address of the corresponding packet. Return a null pointer if the
        //! @a index is out of range or if the packet was previously dropped.
        //!
        TSPacket* packet(size_t index) const;

        //!
        //! Get the address of the metadata of a packet inside the window.
        //! @param [in] index Index of the packet inside the window, from 0 to size()-1.
        //! @return The address of the corresponding packet metadata. Return a null pointer if the
        //! @a index is out of range or if the packet was previously dropped.
        //!
        TSPacketMetadata* metadata(size_t index) const;

        //!
        //! Check if a packet inside the window is a null packet.
        //! @param [in] index Index of the packet inside the window, from 0 to size()-1.
        //! @return True if the corresponding packet is valid and a null packet, false otherwise.
        //!
        bool isNullPacket(size_t index) const;

        //!
        //! Get the address of a packet and its metadata inside the window.
        //! @param [in] index Index of the packet inside the window, from 0 to size()-1.
        //! @param [out] packet The address of the corresponding packet.
        //! @param [out] metadata The address of the corresponding packet metadata.
        //! @return True on success, false if the @a index is out of range or if the packet was previously dropped.
        //! In the latter case, @a packet and @a metadata are both null pointers.
        //!
        bool get(size_t index, TSPacket*& packet, TSPacketMetadata*& metadata) const;

        //!
        //! Get the physical index of a packet inside a buffer.
        //! @param [in] index Index of the packet inside the window, from 0 to size()-1.
        //! @param [in] buffer Base address of a packet buffer. The packet window must be a view over that buffer.
        //! @param [in] buffer_size Number of TS packets in the buffer.
        //! @return The index of the packet in the buffer or NPOS if out of range.
        //!
        size_t packetIndexInBuffer(size_t index, const TSPacket* buffer, size_t buffer_size) const;

        //!
        //! Nullify the packet at the corresponding index.
        //! @param [in] index Index of the packet inside the windows, from 0 to size()-1.
        //!
        void nullify(size_t index);

        //!
        //! Drop the packet at the corresponding index.
        //! Internally, the sync byte of the packet in the buffer is zeroed and the packet is no longer usable.
        //! @param [in] index Index of the packet inside the windows, from 0 to size()-1.
        //!
        void drop(size_t index);

        //!
        //! Get the number of nullified packets.
        //! @return The number of nullified packets in this window. Packets which were already null
        //! before calling nullify() are not counted. Multiple calls to nullify() on the same packet
        //! are counted once only.
        //!
        size_t nullifyCount() const { return _nullify_count; }

        //!
        //! Get the number of dropped packets.
        //! @return The number of dropped packets in this window. Multiple calls to drop() on the same packet are counted once only.
        //!
        size_t dropCount() const { return _drop_count; }

        //!
        //! Get the number of contiguous segments of packets (informational only).
        //! @return The number of contiguous segments of packets.
        //!
        size_t segmentCount() const { return _ranges.size(); }

    private:
        // This class describes a physically contiguous range of TS packets.
        class PacketRange
        {
        public:
            TSPacket*         packets;   // Address of first TS packet in this range.
            TSPacketMetadata* metadata;  // Address of first TS packet metadata in this range.
            size_t            first;     // Index of first TS packet in this range.
            size_t            count;     // Number of TS packets in this range.
        };

        // Same as public get() but returns non-null addresses for dropped packets.
        bool getInternal(size_t index, TSPacket*& packet, TSPacketMetadata*& metadata) const;

        size_t                   _size;              // Number of packets in the window.
        size_t                   _nullify_count;     // Number of nullified packets.
        size_t                   _drop_count;        // Number of dropped packets.
        mutable volatile size_t  _last_range_index;  // Last accessed range (to optimiza sequential access).
        std::vector<PacketRange> _ranges;            // Ranges of contiguous packets.
    };
}

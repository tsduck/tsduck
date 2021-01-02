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
        TS_NOBUILD_NOCOPY(TSPacketWindow);
    public:
        //!
        //! This class describes a physically contiguous range of TS packets.
        //! A TSPacketWindow is built from a vector of these ranges.
        //!
        class PacketRange
        {
        public:
            TSPacket*         packets;   //!< Address of first TS packet in this range.
            TSPacketMetadata* metadata;  //!< Address of first TS packet metadata in this range.
            size_t            count;     //!< Number of TS packets in this range.
        };

        //!
        //! A vector of PacketRange.
        //!
        typedef std::vector<PacketRange> PacketRangeVector;

        //!
        //! Constructor.
        //! @param [in] ranges The list of physically contiguous ranges of TS packets.
        //! The logical packet window is made of all those packets from all ranges.
        //! All addresses must be valid, null pointers are not allowed.
        //!
        TSPacketWindow(const PacketRangeVector& ranges);

        //!
        //! Get the number of packets in this window.
        //! @return The number of packets in this window.
        //!
        size_t size() const { return _size; }

        //!
        //! Get the address of a packet inside the window.
        //! @param [in] index Index of the packet inside the windows, from 0 to size()-1.
        //! @return The address of the corresponding packet. Return a null pointer if the
        //! @a index is out of range or if the packet was previously dropped.
        //!
        TSPacket* packet(size_t index) const;

        //!
        //! Get the address of the metadata of a packet inside the window.
        //! @param [in] index Index of the packet inside the windows, from 0 to size()-1.
        //! @return The address of the corresponding packet metadata. Return a null pointer if the
        //! @a index is out of range or if the packet was previously dropped.
        //!
        TSPacketMetadata* metadata(size_t index) const;

        //!
        //! Get the address of a packet and its metadata inside the window.
        //! @param [in] index Index of the packet inside the windows, from 0 to size()-1.
        //! @param [out] packet The address of the corresponding packet.
        //! @param [out] metadata The address of the corresponding packet metadata.
        //! @return True on success, false if the @a index is out of range or if the packet was previously dropped.
        //! In the latter case, @a packet and @a metadata are both null pointers.
        //!
        bool get(size_t index, TSPacket*& packet, TSPacketMetadata*& metadata) const;

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

    private:
        // Extension of packet range with first index of the range.
        class InternalPacketRange: public PacketRange
        {
        public:
            size_t first;
        };

        size_t _size;
        size_t _nullify_count;
        size_t _drop_count;
        mutable volatile size_t _last_range_index;
        std::vector<InternalPacketRange> _ranges;
    };
}

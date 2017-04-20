//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Transport stream file input with seekable buffer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFileInput.h"

namespace ts {

    class TSDUCKDLL TSFileInputBuffered: public TSFileInput
    {
    public:
        // Constructor / destructor.
        // The buffer size is a number of TS packets.
        TSFileInputBuffered (size_t buffer_size);
        virtual ~TSFileInputBuffered();

        // Minimum buffer size. Used to minimize buffer_size in constructor and setBufferSize().
        static const size_t MIN_BUFFER_SIZE = 16;

        // Set the buffer size. Can be done only when the file is closed.
        bool setBufferSize (size_t buffer_size, ReportInterface&);

        // Get the buffer size, the number of packets in the buffer.
        size_t getBufferSize() const {return _buffer.size();}
        size_t getBufferFreeSize() const {return _buffer.size() - _total_count;}
        size_t getBufferedCount() const {return isOpen() ? _total_count : 0;}

        // Open file.
        // Override TSFileInput::open(). There is no rewindable version.
        bool open (const std::string& filename, size_t repeat_count, uint64_t start_offset, ReportInterface&);

        // Read TS packets.
        // Override TSFileInput::read().
        // Return the actual number of read packets.
        // Returning zero means error or end of file repetition.
        size_t read (TSPacket*, size_t max_packets, ReportInterface&);

        // Get the seekable distances inside the buffer.
        // The minimum guaranteed seekable distances are:
        // - Backward seek: The buffer size from the highest previously read packet or
        //   the beginning of file, whichever comes first,
        // - Forward seek: The highest previously read packet, before backward seek.
        size_t getBackwardSeekableCount() const {return isOpen() ? _current_offset : 0;}
        size_t getForwardSeekableCount() const {return isOpen() ? _total_count - _current_offset : 0;}

        // Seek the file backward or forward the specified number of packets.
        bool seekBackward (size_t packet_count, ReportInterface&);
        bool seekForward (size_t packet_count, ReportInterface&);

        // Return the number of read packets
        // Override TSFileInput::getPacketCount().
        // Make sure we do not report packets twice.
        PacketCounter getPacketCount() const;

        // Check if we can seek to the specified absolute position
        // (the "position" is the getPacketCount() value).
        bool canSeek (PacketCounter) const;

        // Seek to the specified absolute position, if it is inside the buffer.
        bool seek (PacketCounter, ReportInterface&);

    private:
        TSPacketVector _buffer;         // Seekable packet circular buffer.
        size_t         _first_index;    // Index of first packet in buffer.
        size_t         _current_offset; // Offset from _first_index of "current" readable packet
        size_t         _total_count;    // Total count of valid packets in buffer.

        // Inaccessible operations
        TSFileInputBuffered (const TSFileInputBuffered&);
        TSFileInputBuffered& operator= (const TSFileInputBuffered&);
        bool rewind (ReportInterface&);
    };
}

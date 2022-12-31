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
//!  Read/write TS packets on a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractReadStreamInterface.h"
#include "tsAbstractWriteStreamInterface.h"
#include "tsTSPacketFormat.h"
#include "tsTSPacketMetadata.h"
#include "tsTSPacket.h"
#include "tsEnumeration.h"

namespace ts {

    class TSPacketMetadata;

    //!
    //! Read/write TS packets on a stream.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSPacketStream
    {
        TS_NOCOPY(TSPacketStream);
    public:
        //!
        //! Constructor.
        //! @param [in] format Initial packet format.
        //! @param [in] reader Reader interface. If null, all read operations will fail.
        //! @param [in] writer Writer interface. If null, all write operations will fail.
        //!
        TSPacketStream(TSPacketFormat format = TSPacketFormat::AUTODETECT, AbstractReadStreamInterface* reader = nullptr, AbstractWriteStreamInterface* writer = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~TSPacketStream();

        //!
        //! Read TS packets from the stream.
        //! @param [out] buffer Address of reception packet buffer.
        //! @param [out] metadata Optional packet metadata. If the file format provides
        //! time stamps, they are set in the metadata. Ignored if null pointer.
        //! @param [in] max_packets Size of @a buffer in packets.
        //! Also size of @a metadata in number of objects (when specified).
        //! @param [in,out] report Where to report errors.
        //! @return The actual number of read packets. Returning zero means error or end of stream.
        //!
        virtual size_t readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets, Report& report);

        //!
        //! Write TS packets to the stream.
        //! @param [in] buffer Address of first packet to write.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp. Otherwise, the last
        //! written timestamp is repeated.
        //! @param [in] packet_count Number of packets to write.
        //! Also size of @a metadata in number of objects (when specified).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, Report& report);

        //!
        //! Get the number of read packets.
        //! @return The number of read packets.
        //!
        PacketCounter readPacketsCount() const { return _total_read; }

        //!
        //! Get the number of written packets.
        //! @return The number of written packets.
        //!
        PacketCounter writePacketsCount() const { return _total_write; }

        //!
        //! Maximum size in bytes of a packet header for non-TS format.
        //! Must be lower than the TS packet size to allow auto-detection on read.
        //!
        static constexpr size_t MAX_HEADER_SIZE = ts::TSPacketMetadata::SERIALIZATION_SIZE;

        //!
        //! Maximum size in bytes of a packet trailer for non-TS format.
        //! Must be lower than the TS packet size to allow auto-detection on read.
        //!
        static constexpr size_t MAX_TRAILER_SIZE = ts::RS_SIZE;

        //!
        //! Get the packet header size, based on the packet format.
        //! This "header" comes before the classical 188-byte TS packet.
        //! @return The packet header size in bytes (before the TS packet).
        //!
        size_t packetHeaderSize() const;

        //!
        //! Get the packet trailer size, based on the packet format.
        //! This "trailer" comes after the classical 188-byte TS packet.
        //! @return The packet trailer size in bytes (before the TS packet).
        //!
        size_t packetTrailerSize() const;

        //!
        //! Get the file format.
        //! @return The file format.
        //!
        TSPacketFormat packetFormat() const { return _format; }

        //!
        //! Get the file format as a string.
        //! @return The file format as a string.
        //!
        UString packetFormatString() const { return TSPacketFormatEnum.name(_format); }

    protected:
        //!
        //! Reset the stream format and counters.
        //! @param [in] format Initial packet format.
        //! @param [in] reader Reader interface. If null, all read operations will fail.
        //! @param [in] writer Writer interface. If null, all write operations will fail.
        //!
        void resetPacketStream(TSPacketFormat format, AbstractReadStreamInterface* reader, AbstractWriteStreamInterface* writer);

        PacketCounter _total_read;   //!< Total read packets.
        PacketCounter _total_write;  //!< Total written packets.

    private:
        TSPacketFormat                _format;
        AbstractReadStreamInterface*  _reader;
        AbstractWriteStreamInterface* _writer;
        uint64_t _last_timestamp;             // Last write time stamp in PCR units (M2TS files).
        size_t   _trail_size;                 // Number of meaningful bytes in _trail
        uint8_t  _trail[MAX_TRAILER_SIZE+1];  // Transient buffer for auto-detection of trailer
    };
}

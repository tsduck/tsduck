//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Read/write TS packets on a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractStream.h"
#include "tsTSPacketFormat.h"
#include "tsTSPacketMetadata.h"
#include "tsTSPacket.h"
#include "tsNames.h"

namespace ts {

    class TSPacketMetadata;

    //!
    //! Read/write TS packets on a stream.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSPacketStream
    {
        TS_NOBUILD_NOCOPY(TSPacketStream);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reporter Reference to an object providing a Report.
        //! This reference is kept in the constructed object instance.
        //! @param [in] format Initial packet format.
        //! @param [in] stream Stream read/write interface. Can be null and replaced later.
        //!
        TSPacketStream(ReporterInterface& reporter, TSPacketFormat format = TSPacketFormat::AUTODETECT, AbstractStream* stream = nullptr);

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
        //! @return The actual number of read packets. Returning zero means error or end of stream.
        //!
        virtual size_t readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets);

        //!
        //! Write TS packets to the stream.
        //! @param [in] buffer Address of first packet to write.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp. Otherwise, the last
        //! written timestamp is repeated.
        //! @param [in] packet_count Number of packets to write.
        //! Also size of @a metadata in number of objects (when specified).
        //! @return True on success, false on error.
        //!
        virtual bool writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count);

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
        UString packetFormatString() const { return TSPacketFormatEnum().name(_format); }

    protected:
        //!
        //! Reset the stream format and counters.
        //! @param [in] format Initial packet format.
        //! @param [in] stream Stream read/write interface.
        //!
        void resetPacketStream(TSPacketFormat format, AbstractStream* stream);

        PacketCounter _total_read = 0;   //!< Total read packets.
        PacketCounter _total_write = 0;  //!< Total written packets.

    private:
        ReporterInterface& _reporter;
        TSPacketFormat     _format = TSPacketFormat::TS;
        AbstractStream*    _stream = nullptr;
        PCR                _last_timestamp {};              // Last write time stamp in PCR units (M2TS files).
        size_t             _trail_size = 0;                 // Number of meaningful bytes in _trail
        uint8_t            _trail[MAX_TRAILER_SIZE+1] {};   // Transient buffer for auto-detection of trailer
    };
}

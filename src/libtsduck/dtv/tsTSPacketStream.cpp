//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsTSPacketStream.h"
#include "tsTSPacketMetadata.h"
TSDUCK_SOURCE;

// Maximum size of a packet header for non-TS format.
// Must be lower than the TS packet size to allow auto-detection on read.
namespace {
    constexpr size_t MAX_HEADER_SIZE = ts::TSPacketMetadata::SERIALIZATION_SIZE;
}


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSPacketStream::TSPacketStream(TSPacketFormat format, AbstractReadStreamInterface* reader, AbstractWriteStreamInterface* writer) :
    _total_read(0),
    _total_write(0),
    _format(format),
    _reader(reader),
    _writer(writer),
    _last_timestamp(0)
{
}

ts::TSPacketStream::~TSPacketStream()
{
}


//----------------------------------------------------------------------------
// Reset the stream format and counters.
//----------------------------------------------------------------------------

void ts::TSPacketStream::resetPacketStream(TSPacketFormat format, AbstractReadStreamInterface* reader, AbstractWriteStreamInterface* writer)
{
    _total_read = 0;
    _total_write = 0;
    _format = format;
    _reader = reader;
    _writer = writer;
    _last_timestamp = 0;
}


//----------------------------------------------------------------------------
// Get header size in bytes before a packet.
//----------------------------------------------------------------------------

size_t ts::TSPacketStream::packetHeaderSize() const
{
    switch (_format) {
        case TSPacketFormat::AUTODETECT: return 0;
        case TSPacketFormat::TS:         return 0;
        case TSPacketFormat::M2TS:       return 4;
        case TSPacketFormat::DUCK:       return TSPacketMetadata::SERIALIZATION_SIZE;
        default:                         return 0;
    }
}


//----------------------------------------------------------------------------
// Read TS packets. Return the actual number of read packets.
//----------------------------------------------------------------------------

size_t ts::TSPacketStream::readPackets(TSPacket *buffer, TSPacketMetadata *metadata, size_t max_packets, Report &report)
{
    if (_reader == nullptr) {
        report.error(u"internal error, cannot read TS packets from this stream");
        return 0;
    }

    // Number of read packets.
    size_t read_packets = 0;
    size_t read_size = 0;

    // Header buffer for M2TS or M8TS formats.
    uint8_t header[MAX_HEADER_SIZE];
    size_t header_size = packetHeaderSize();
    assert(header_size <= sizeof(header));

    // If format is autodetect, read one packet to check where the sync byte is.
    if (_format == TSPacketFormat::AUTODETECT) {

        // Read one packet.
        if (!_reader->readStreamComplete(buffer, PKT_SIZE, read_size, report) || read_size < PKT_SIZE) {
            return 0; // less than one packet in that file
        }

        // Metadata for first packet (if there is a header).
        TSPacketMetadata mdata;

        // Check the position of the 0x47 sync byte to detect a potential header.
        if (buffer->b[0] == SYNC_BYTE) {
            // No header (or header starting with 0x47...)
            _format = TSPacketFormat::TS;
        }
        else if (buffer->b[4] == SYNC_BYTE) {
            _format = TSPacketFormat::M2TS;
            mdata.setInputTimeStamp(GetUInt32(buffer) & 0x3FFFFFFF, SYSTEM_CLOCK_FREQ, TimeSource::M2TS);
        }
        else if (buffer->b[0] == TSPacketMetadata::SERIALIZATION_MAGIC && buffer->b[TSPacketMetadata::SERIALIZATION_SIZE] == SYNC_BYTE) {
            _format = TSPacketFormat::DUCK;
            mdata.deserialize(buffer->b, TSPacketMetadata::SERIALIZATION_SIZE);
        }
        else {
            report.error(u"cannot detect TS file format");
            return 0;
        }
        report.debug(u"detected TS file format %s", {packetFormatString()});

        // If there was a header, remove it and read the rest of the packet.
        header_size = packetHeaderSize();
        assert(header_size <= sizeof(header));
        if (header_size > 0) {
            // memmove() can move overlapping areas.
            char* data = reinterpret_cast<char*>(buffer);
            ::memmove(data, data + header_size, PKT_SIZE - header_size);
            if (!_reader->readStreamComplete(data + PKT_SIZE - header_size, header_size, read_size, report) || read_size < header_size) {
                return 0; // less than one packet in that file
            }
        }

        // Now we have read the first packet.
        read_packets++;
        buffer++;
        max_packets--;
        if (metadata != nullptr) {
            *metadata++ = mdata;
        }
    }

    // Repeat reading packets until the buffer is full or error.
    // Rewind on end of file if repeating is set.
    bool success = true;
    while (success && max_packets > 0 && !_reader->endOfStream()) {

        switch (_format) {
            case TSPacketFormat::AUTODETECT: {
                // Should not get there.
                assert(false);
                return 0;
            }
            case TSPacketFormat::TS: {
                // Bulk read in TS format.
                success = _reader->readStreamComplete(buffer, max_packets * PKT_SIZE, read_size, report);
                // Count packets. Truncate incomplete packets at end of file.
                const size_t count = read_size / PKT_SIZE;
                assert(count <= max_packets);
                read_packets += count;
                buffer += count;
                max_packets -= count;
                if (metadata != nullptr) {
                    TSPacketMetadata::Reset(metadata, count);
                    metadata += count;
                }
                break;
            }
            case TSPacketFormat::M2TS:
            case TSPacketFormat::DUCK: {
                // Read header + packet.
                success = _reader->readStreamComplete(header, header_size, read_size, report);
                if (success && read_size == header_size) {
                    success = _reader->readStreamComplete(buffer, PKT_SIZE, read_size, report);
                    if (success && read_size == PKT_SIZE) {
                        read_packets++;
                        buffer++;
                        max_packets--;
                        if (metadata != nullptr) {
                            if (_format == TSPacketFormat::M2TS) {
                                metadata->reset();
                                metadata->setInputTimeStamp(GetUInt32(header) & 0x3FFFFFFF, SYSTEM_CLOCK_FREQ, TimeSource::M2TS);
                            }
                            else {
                                metadata->deserialize(header, TSPacketMetadata::SERIALIZATION_SIZE);
                            }
                            metadata++;
                        }
                    }
                }
                break;
            }
            default: {
                report.error(u"internal error, invalid TS file format %s", {packetFormatString()});
                return 0;
            }
        }
    }

    // Return the number of input packets.
    _total_read += read_packets;
    return read_packets;
}


//----------------------------------------------------------------------------
// Write TS packets.
//----------------------------------------------------------------------------

bool ts::TSPacketStream::writePackets(const TSPacket *buffer, const TSPacketMetadata *metadata, size_t packet_count, Report &report)
{
    if (_writer == nullptr) {
        report.error(u"internal error, cannot write TS packets to this stream");
        return 0;
    }

    bool success = true;

    switch (_format) {
        case TSPacketFormat::AUTODETECT:
        case TSPacketFormat::TS: {
            // If file format is not yet known, force it as TS, the default.
            _format = TSPacketFormat::TS;
            // Bulk write in TS format.
            size_t written_size = 0;
            success = _writer->writeStream(buffer, packet_count * PKT_SIZE, written_size, report);
            _total_write += written_size / PKT_SIZE;
            break;
        }
        case TSPacketFormat::M2TS:
        case TSPacketFormat::DUCK: {
            // Write header + packet, packet by packet.
            uint8_t header[MAX_HEADER_SIZE];
            const size_t header_size = packetHeaderSize();
            for (size_t i = 0; success && i < packet_count; ++i) {
                // Get time stamp of current packet or reuse last one.
                if (metadata != nullptr && metadata[i].hasInputTimeStamp()) {
                    _last_timestamp = metadata[i].getInputTimeStamp();
                }
                // Build header.
                if (_format == TSPacketFormat::M2TS) {
                    // 30-bit time stamp in PCR units (2 most-significant bits are copy-control).
                    PutUInt32(header, uint32_t(_last_timestamp & 0x3FFFFFFF));
                }
                else if (metadata != nullptr) {
                    // DUCK format with application-provided metadata.
                    metadata[i].serialize(header, sizeof(header));
                }
                else {
                    // DUCK format with default metadata.
                    TSPacketMetadata mdata;
                    mdata.serialize(header, sizeof(header));
                }
                // Write header, then packet.
                size_t written_size = 0;
                success = _writer->writeStream(header, header_size, written_size, report) &&
                          _writer->writeStream(&buffer[i], PKT_SIZE, written_size, report);
                if (success) {
                    _total_write++;
                }
            }
            break;
        }
        default: {
            report.error(u"internal error, invalid TS file format %s", {packetFormatString()});
            return false;
        }
    }

    return success;
}

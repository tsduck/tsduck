//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFile.h"
#include "tsTSPacketMetadata.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TSFile::TSFile(Report* report) :
    BinaryFile(report, false),
    TSPacketStream(*static_cast<ReporterBase*>(this))
{
}

ts::TSFile::TSFile(ReporterBase* delegate) :
    BinaryFile(delegate, false),
    TSPacketStream(*static_cast<ReporterBase*>(this))
{
}

ts::TSFile::~TSFile()
{
    if (isOpen()) {
        // Close here, don't let the superclass close it, it won't write stuffing packets.
        TSFile::close(true);
    }
}


//----------------------------------------------------------------------------
// Set initial and final artificial stuffing.
//----------------------------------------------------------------------------

void ts::TSFile::setStuffing(size_t initial, size_t final)
{
    _open_null = initial;
    _close_null = final;
}


//----------------------------------------------------------------------------
// Initialize TS-specific state over open.
//----------------------------------------------------------------------------

bool ts::TSFile::wrapOpen(bool open_status, TSPacketFormat format)
{
    // If open was successful.
    if (open_status) {

        // Reset TS-specific state.
        resetPacketStream(format, this);
        _open_null_read = _open_null;
        _close_null_read = _close_null;

        // In write mode, write initial null packets.
        if ((getFlags() & WRITE) != 0 && _open_null > 0 && !writeStuffing(_open_null)) {
            close(true);
            open_status = false;
        }
    }
    return open_status;
}


//----------------------------------------------------------------------------
// Open file in different flavors.
//----------------------------------------------------------------------------

// Open file for read in a rewindable mode.
bool ts::TSFile::openRead(const fs::path& filename, uint64_t start_offset, TSPacketFormat format)
{
    return wrapOpen(BinaryFile::openRead(filename, start_offset), format);
}

// Open file for read with optional repetition.
bool ts::TSFile::openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset, TSPacketFormat format)
{
    return wrapOpen(BinaryFile::openRead(filename, repeat_count, start_offset), format);
}

// Open file, generic form. TSFile version with explicit format.
bool ts::TSFile::open(const fs::path& filename, OpenFlags flags, TSPacketFormat format)
{
    return wrapOpen(BinaryFile::open(filename, flags), format);
}

// Open file, generic form. Inherited version from BinaryFile, format is AUTODETECT.
bool ts::TSFile::open(const fs::path& filename, OpenFlags flags)
{
    return wrapOpen(BinaryFile::open(filename, flags), TSPacketFormat::AUTODETECT);
}


//----------------------------------------------------------------------------
// Check end of file.
//----------------------------------------------------------------------------

bool ts::TSFile::endOfStream()
{
    // Return an end-of-file when there is no more close stuffing packet to return.
    return BinaryFile::endOfStream() && _close_null_read == 0;
}

//----------------------------------------------------------------------------
// Seek the file to the specified packet_index plus the start_offset.
//----------------------------------------------------------------------------

bool ts::TSFile::seekPacket(PacketCounter packet_index)
{
    return seekByte(packet_index * (packetHeaderSize() + PKT_SIZE));
}


//----------------------------------------------------------------------------
// Close file.
//----------------------------------------------------------------------------

bool ts::TSFile::close(bool silent)
{
    // In write mode, write final null packets.
    if ((getFlags() & WRITE) != 0 && _close_null > 0) {
        writeStuffing(_close_null);
    }

    // Close the file using the superclass.
    return BinaryFile::close(silent);
}


//----------------------------------------------------------------------------
// Read TS packets. Return the actual number of read packets.
// Override TSPacketStream implementation
//----------------------------------------------------------------------------

size_t ts::TSFile::readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets)
{
    size_t ret_count = 0;

    // Initial artificial stuffing.
    if (_open_null_read > 0 && max_packets > 0) {
        const size_t count = std::min(max_packets, _open_null_read);
        report().debug(u"reading %d starting null packets", count);
        readStuffing(buffer, metadata, count);
        _total_read += count;
        ret_count += count;
        max_packets -= count;
        _open_null_read -= count;
    }

    // Repeat reading packets until the buffer is full or error.
    // Rewind on end of file if repeating is set.
    while (max_packets > 0 && !BinaryFile::endOfStream()) {

        // Invoke superclass.
        const size_t count = TSPacketStream::readPackets(buffer, metadata, max_packets);

        if (count == 0 && !BinaryFile::endOfStream()) {
            break; // actual error
        }

        // Accumulate packets.
        ret_count += count;
        buffer += count;
        max_packets -= count;
        if (metadata != nullptr) {
            metadata += count;
        }
    }

    // Final artificial stuffing.
    if (BinaryFile::endOfStream() && _close_null_read > 0 && max_packets > 0) {
        const size_t count = std::min(max_packets, _close_null_read);
        report().debug(u"reading %d stopping null packets", count);
        readStuffing(buffer, metadata, count);
        _total_read += count;
        ret_count += count;
        _close_null_read -= count;
    }

    return ret_count;
}


//----------------------------------------------------------------------------
// Read/write artificial stuffing.
//----------------------------------------------------------------------------

void ts::TSFile::readStuffing(TSPacket*& buffer, TSPacketMetadata*& metadata, size_t count)
{
    while (count-- > 0) {
        *buffer++ = NullPacket;
        if (metadata != nullptr) {
            metadata->reset();
            (metadata++)->setInputStuffing(true);
        }
    }
}

bool ts::TSFile::writeStuffing(size_t count)
{
    TSPacketMetadata mdata;
    mdata.setInputStuffing(true);
    for (size_t i = 0; i < count; ++i) {
        if (!writePackets(&NullPacket, &mdata, 1)) {
            return false;
        }
    }
    return true;
}

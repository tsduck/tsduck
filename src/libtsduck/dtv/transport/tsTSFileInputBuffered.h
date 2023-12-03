//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream file input with seekable buffer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFile.h"
#include "tsTSPacketMetadata.h"

namespace ts {
    //!
    //! Transport stream file input with a seekable buffer.
    //! @ingroup mpeg
    //!
    //! This variant of TSFile allows to seek back and forth to some extent
    //! without doing I/O's and can work on non-seekable files (pipes for instance).
    //!
    class TSDUCKDLL TSFileInputBuffered: public TSFile
    {
        TS_NOBUILD_NOCOPY(TSFileInputBuffered);
    public:
        //!
        //! Constructor.
        //! @param [in] buffer_size Size of the seekable buffer in number of TS packets.
        //!
        TSFileInputBuffered(size_t buffer_size);

        //!
        //! Destructor.
        //!
        virtual ~TSFileInputBuffered() override;

        //!
        //! Minimum buffer size.
        //! Used to minimize @a buffer_size in constructor and setBufferSize().
        //!
        static constexpr size_t MIN_BUFFER_SIZE = 16;

        //!
        //! Set the buffer size.
        //! Can be done only when the file is closed.
        //! @param [in] buffer_size Size of the seekable buffer in number of TS packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool setBufferSize(size_t buffer_size, Report& report);

        //!
        //! Get the buffer size.
        //! @return The buffer size in number of TS packets.
        //!
        size_t getBufferSize() const { return _buffer.size(); }

        //!
        //! Get the size of the free space in the buffer.
        //! @return The number of free TS packets in the buffer.
        //!
        size_t getBufferFreeSize() const { return _buffer.size() - _total_count; }

        //!
        //! Get the number of TS packets in the buffer.
        //! @return The number of TS packets in the buffer.
        //!
        size_t getBufferedCount() const { return isOpen() ? _total_count : 0; }

        //!
        //! Open the file.
        //! Override TSFile::openRead(). There is no rewindable version.
        //! @param [in] filename File name. If empty, use standard input.
        //! Must be a regular file is @a repeat_count is not 1 or if
        //! @a start_offset is not zero.
        //! @param [in] repeat_count Reading packets loops back after end of
        //! file until all repeat are done. If zero, infinitely repeat.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets at each iteration.
        //! @param [in,out] report Where to report errors.
        //! @param [in] format Expected format of the TS file.
        //! @return True on success, false on error.
        //!
        bool openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset, Report& report, TSPacketFormat format = TSPacketFormat::AUTODETECT);

        //!
        //! Read TS packets.
        //! Override TSFile::read().
        //! If the file file was opened with a @a repeat_count different from 1,
        //! reading packets transparently loops back at end if file.
        //! @param [out] buffer Address of reception packet buffer.
        //! @param [in] max_packets Size of @a buffer in packets.
        //! @param [in,out] report Where to report errors.
        //! @param [in,out] metadata Optional packet metadata. If the file format provides
        //! time stamps, they are set in the metadata. Ignored if null pointer.
        //! @return The actual number of read packets. Returning zero means
        //! error or end of file repetition.
        //!
        size_t read(TSPacket* buffer, size_t max_packets, Report& report, TSPacketMetadata* metadata = nullptr);

        //!
        //! Get the backward seekable distance inside the buffer.
        //! This is the minimum guaranteed seekable distance.
        //! @return The buffer size from the highest previously read packet or
        //! the beginning of file, whichever comes first.
        //!
        size_t getBackwardSeekableCount() const { return isOpen() ? _current_offset : 0; }

        //!
        //! Get the forward seekable distance inside the buffer.
        //! This is the minimum guaranteed seekable distance.
        //! @return  The highest previously read packet index, before backward seek.
        //!
        size_t getForwardSeekableCount() const { return isOpen() ? _total_count - _current_offset : 0; }

        //!
        //! Seek the file backward the specified number of packets.
        //! @param [in] packet_count The number of packets to seek
        //! backward from the current position.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool seekBackward(size_t packet_count, Report& report);

        //!
        //! Seek the file forward the specified number of packets.
        //! @param [in] packet_count The number of packets to seek
        //! forward from the current position.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool seekForward(size_t packet_count, Report& report);

        //!
        //! Get the number of read packets.
        //! Override TSFile::readPacketsCount().
        //! @return The number of read packets.
        //!
        PacketCounter readPacketsCount() const;

        //!
        //! Check if we can seek to the specified absolute position.
        //! @param [in] position Absolute packet index in the file.
        //! @return True if the specified @a position is inside the buffer.
        //!
        bool canSeek(PacketCounter position) const;

        //!
        //! Seek to the specified absolute position, if it is inside the buffer.
        //! @param [in] position Absolute packet index in the file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool seek(PacketCounter position, Report& report);

    private:
        TSPacketVector         _buffer;             // Seekable packet circular buffer.
        TSPacketMetadataVector _metadata;           // Seekable packet metadata circular buffer.
        size_t                 _first_index = 0;    // Index of first packet in buffer.
        size_t                 _current_offset = 0; // Offset from _first_index of "current" readable packet
        size_t                 _total_count = 0;    // Total count of valid packets in buffer.

        // Make sure that the generic open() returns an error.
        virtual bool open(const fs::path& filename, OpenFlags flags, Report& report, TSPacketFormat format) override;

        // Make rewind inaccessible.
        bool rewind(Report&) = delete;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A TS packet buffer for time shift.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsTSFile.h"
#include "tsTSPacketMetadata.h"
#include "tsReport.h"

namespace ts {

    class TSPacketMetadata;

    //!
    //! A TS packet buffer for time shift.
    //! The buffer is partly implemented in virtual memory and partly on disk.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TimeShiftBuffer
    {
        TS_NOCOPY(TimeShiftBuffer);
    public:
        //!
        //! Minimum size in packets of a time shift buffer.
        //!
        static constexpr size_t MIN_TOTAL_PACKETS = 2;
        //!
        //! Default size in packets of a time shift buffer.
        //!
        static constexpr size_t DEFAULT_TOTAL_PACKETS = 128;
        //!
        //! Minimum number of cached packets in memory.
        //!
        static constexpr size_t MIN_MEMORY_PACKETS = 2;
        //!
        //! Default number of cached packets in memory.
        //!
        static constexpr size_t DEFAULT_MEMORY_PACKETS = 128;

        //!
        //! Constructor.
        //! @param [in] count Max number of packets in the buffer.
        //!
        TimeShiftBuffer(size_t count = DEFAULT_TOTAL_PACKETS);

        //!
        //! Destructor.
        //!
        virtual ~TimeShiftBuffer();

        //!
        //! Set the total size of the time shift buffer in packets.
        //! Must be called before open().
        //! @param [in] count Max number of packets in the buffer.
        //! @return True on success, false if already open.
        //!
        bool setTotalPackets(size_t count);

        //!
        //! Set the maximum number of cached packets to be held in memory.
        //! Must be called before open().
        //! @param [in] count Max number of cached packets in memory.
        //! @return True on success, false if already open.
        //!
        bool setMemoryPackets(size_t count);

        //!
        //! Set the directory for the backup file on disk.
        //! Must be called before open().
        //! By default, the file is created in the system-dependent temporary directory.
        //! When the maximum number of cached packets in memory is larger than the
        //! buffer size, the buffer is entirely resident in memory and no file is created.
        //! The back file is automatically deleted when the time-shift buffer is closed.
        //! @param [in] directory Directory name.
        //! @return True on success, false if already open or too small.
        //!
        bool setBackupDirectory(const fs::path& directory);

        //!
        //! Open the buffer.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(Report& report);

        //!
        //! Close the buffer.
        //! The memory is freed and the disk backup file is deleted.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Check if the buffer is open.
        //! @return True if the buffer is open.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Get the total size in packets of the time-shift buffer.
        //! @return The total size in packets of the time-shift buffer.
        //!
        size_t size() const { return _total_packets; }

        //!
        //! Get the current number of packets in the time-shift buffer.
        //! @return The current number of packets in the time-shift buffer.
        //!
        size_t count() const { return _cur_packets; }

        //!
        //! Check if the buffer is empty.
        //! @return True when the buffer is empty, false otherwise.
        //!
        bool empty() const { return _cur_packets == 0; }

        //!
        //! Check if the buffer is full.
        //! @return True when the buffer is full, false otherwise.
        //!
        bool full() const { return _cur_packets >= _total_packets; }

        //!
        //! Check if the buffer is completely memory resident.
        //! @return True when the buffer is memory resident, false when it is backup by a file.
        //!
        bool memoryResident() const { return _total_packets <= _mem_packets; }

        //!
        //! Push a packet in the time-shift buffer and pull the oldest one.
        //!
        //! As long as the buffer is not full, a null packet is returned.
        //! When the buffer is full, the oldest packet is returned and removed
        //! from the buffer. Initial null packets which are generated while the
        //! time-shift buffer is filling can be recognized as they are marked as
        //! "input stuffing" in their metadata, after returning from shift().
        //!
        //! @param [in,out] packet On input, contains the packet to push.
        //! On output, contains the time-shifted packet.
        //! @param [in,out] metadata Packet metadata.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool shift(TSPacket& packet, TSPacketMetadata& metadata, Report& report);

    private:
        bool     _is_open = false;          // Buffer is open.
        size_t   _cur_packets = 0;          // Current number of packets in the buffer.
        size_t   _total_packets = DEFAULT_TOTAL_PACKETS; // Total capacity of the buffer.
        size_t   _mem_packets = DEFAULT_MEMORY_PACKETS;  // Max packets in memory.
        fs::path _directory {};             // Where to store the backup file.
        TSFile   _file {};                  // Backup file on disk.
        size_t   _next_read = 0;            // Index in buffer of next packet to read.
        size_t   _next_write = 0;           // Index in buffer of next packet to write.
        size_t   _wcache_next = 0;          // Next index to write in _wcache (up to end of _wcache).
        size_t   _rcache_end = 0;           // End index in _rcache (after last loaded packet).
        size_t   _rcache_next = 0;          // Next index to read in _rcache.
        TSPacketVector         _wcache {};  // Write cache (or complete buffer if in memory).
        TSPacketVector         _rcache {};  // Read cache.
        TSPacketMetadataVector _wmdata {};  // Packet metadata for _wcache.
        TSPacketMetadataVector _rmdata {};  // Packet metadata for _rcache.

        // Seek, read, write in the backup file.
        bool seekFile(size_t index, Report& report);
        bool writeFile(size_t index, const TSPacket* buffer, const TSPacketMetadata* mdata, size_t count, Report& report);
        size_t readFile(size_t index, TSPacket* buffer, TSPacketMetadata* mdata, size_t count, Report& report);
    };
}

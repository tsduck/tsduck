//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeShiftBuffer.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor
//----------------------------------------------------------------------------

ts::TimeShiftBuffer::TimeShiftBuffer(size_t count) :
    _total_packets(std::max(count, MIN_TOTAL_PACKETS))
{
}

ts::TimeShiftBuffer::~TimeShiftBuffer()
{
    close(NULLREP);
}


//----------------------------------------------------------------------------
// Set various characteristics, must be called before open.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::setTotalPackets(size_t count)
{
    if (_is_open) {
        return false;
    }
    else {
        _total_packets = std::max(count, MIN_TOTAL_PACKETS);
        return true;
    }
}

bool ts::TimeShiftBuffer::setMemoryPackets(size_t count)
{
    if (_is_open) {
        return false;
    }
    else {
        _mem_packets = std::max(count, MIN_MEMORY_PACKETS);
        return true;
    }
}

bool ts::TimeShiftBuffer::setBackupDirectory(const fs::path& directory)
{
    if (_is_open) {
        return false;
    }
    else {
        _directory = directory;
        return true;
    }
}


//----------------------------------------------------------------------------
// Open the buffer.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::open(Report& report)
{
    if (_is_open) {
        report.error(u"time-shift buffer already open");
        return false;
    }

    if (memoryResident()) {
        // The buffer is entirely memory-resident in _wcache.
        _wcache.resize(_total_packets);
        _wmdata.resize(_total_packets);
        _rcache.clear();
        _rmdata.clear();
    }
    else {
        // The buffer is backed up on disk.
        // Get the name of a temporary file. If a directory is specified, we will use the base name only.
        fs::path filename(TempFile());
        if (!_directory.empty()) {
            if (fs::is_directory(_directory)) {
                filename = _directory + fs::path::preferred_separator + filename.filename();
            }
            else {
                report.error(u"directory %s does not exist", {_directory});
                return false;
            }
        }

        // Create the backup file. The flag temporary means that it will be deleted on close.
        // Use TSDuck proprietary format to save the packet metadata.
        if (!_file.open(filename, TSFile::READ | TSFile::WRITE | TSFile::TEMPORARY, report, TSPacketFormat::DUCK)) {
            return false;
        }

        // The read and write buffers use half of memory quota each.
        // Since the size of the file is larger than the sum of the two,
        // the read and write caches never overlap when the buffer is full.
        _wcache.resize(_mem_packets / 2);
        _wmdata.resize(_mem_packets / 2);
        _rcache.resize(_mem_packets / 2);
        _rmdata.resize(_mem_packets / 2);
    }

    _cur_packets = 0;
    _next_read = _next_write = 0;
    _wcache_next = _rcache_end = _rcache_next = 0;
    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Close the buffer.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::close(Report& report)
{
    if (!_is_open) {
        return false;
    }

    _is_open = false;
    _cur_packets = 0;
    _wcache.clear();
    _wmdata.clear();
    _rcache.clear();
    _rmdata.clear();
    return !_file.isOpen() || _file.close(report);
}


//----------------------------------------------------------------------------
// Push a packet in the time-shift buffer and pull the oldest one.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::shift(TSPacket& packet, TSPacketMetadata& mdata, Report& report)
{
    if (!_is_open) {
        report.error(u"time-shift buffer not open");
        return false;
    }

    TSPacket ret_packet(NullPacket);
    TSPacketMetadata ret_mdata;
    const bool was_full = full();

    assert(_cur_packets <= _total_packets);
    assert(_next_read < _total_packets);
    assert(_next_write < _total_packets);

    if (memoryResident()) {
        // The buffer is entirely memory-resident in _wcache.
        assert(_wcache.size() == _total_packets);
        if (was_full) {
            // Buffer full: return oldest packet.
            ret_packet = _wcache[_next_read];
            ret_mdata = _wmdata[_next_read];
            _next_read = (_next_read + 1) % _wcache.size();
        }
        else {
            // Buffer not full, increase the packet count.
            _cur_packets++;
        }
        _wcache[_next_write] = packet;
        _wmdata[_next_write] = mdata;
        _next_write = (_next_write + 1) % _wcache.size();
    }
    else {
        // The buffer uses a backup file.
        if (!was_full) {
            // While the buffer is not full, simply write the packet in the file.
            if (!_file.writePackets(&packet, &mdata, 1, report)) {
                return false;
            }
            _cur_packets++;
        }
        else {
            // The buffer is full, now read and write in caches.
            // First, make sure the read cache is filled.
            if (_rcache_next >= _rcache_end) {
                // Read cache is empty, load it.
                const size_t count = std::min(_rcache.size(), _total_packets - _next_read);
                _rcache_next = 0;
                _rcache_end = readFile(_next_read, &_rcache[0], &_rmdata[0], count, report);
                if (_rcache_end == 0) {
                    report.error(u"error reading time-shift file");
                    return false;
                }
            }
            // Return oldest packet from memory cache.
            ret_packet = _rcache[_rcache_next];
            ret_mdata = _rmdata[_rcache_next++];
            _next_read = (_next_read + 1) % _total_packets;
            // Flush the write cache if necessary.
            if (_wcache_next >= _wcache.size()) {
                // Flush the entire write cache on disk.
                // Split in two operations if exceeds the end of file.
                // Write index in file of the start of the write cache:
                const size_t file_index = _next_write >= _wcache.size() ? _next_write - _wcache.size() : _total_packets + _next_write - _wcache.size();
                assert(file_index < _total_packets);
                const size_t count = std::min(_wcache.size(), _total_packets - file_index);
                if (!writeFile(file_index, &_wcache[0], &_wmdata[0], count, report)) {
                    return false;
                }
                // Write second part at begining of file if required.
                if (count < _wcache.size() && !writeFile(0, &_wcache[count], &_wmdata[count], _wcache.size() - count, report)) {
                    return false;
                }
                // Write cache is now empty.
                _wcache_next = 0;
            }
            // Write the next packet in the write cache.
            _wcache[_wcache_next] = packet;
            _wmdata[_wcache_next++] = mdata;
        }
        _next_write = (_next_write + 1) % _total_packets;
    }

    // Returned packet. It is a null packet when the buffer was not yet full.
    if (was_full) {
        packet = ret_packet;
        mdata = ret_mdata;
    }
    else {
        packet = NullPacket;
        mdata.reset();
        mdata.setInputStuffing(true);
    }
    return true;
}


//----------------------------------------------------------------------------
// Seek in the backup file.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::seekFile(size_t index, Report& report)
{
    if (_file.seek(index, report)) {
        return true;
    }
    else {
        report.error(u"error seeking time-shift file at packet index %d", {index});
        return false;
    }
}


//----------------------------------------------------------------------------
// Seek and write in the backup file.
//----------------------------------------------------------------------------

bool ts::TimeShiftBuffer::writeFile(size_t index, const TSPacket* buffer, const TSPacketMetadata* mdata, size_t count, Report& report)
{
    if (!seekFile(index, report)) {
        return false;
    }
    else if (_file.writePackets(buffer, mdata, count, report)) {
        report.debug(u"written %d packets in time-shift file at packet index %d", {count, index});
        return true;
    }
    else {
        report.error(u"error writing %d packets in time-shift file at packet index %d", {count, index});
        return false;
    }
}


//----------------------------------------------------------------------------
// Seek and read in the backup file.
//----------------------------------------------------------------------------

size_t ts::TimeShiftBuffer::readFile(size_t index, TSPacket* buffer, TSPacketMetadata* mdata, size_t count, Report& report)
{
    if (!seekFile(index, report)) {
        return false;
    }
    const size_t retcount = _file.readPackets(buffer, mdata, count, report);
    if (retcount == 0) {
        report.error(u"error reading %d packets in time-shift file at packet index %d", {count, index});
    }
    else {
        report.debug(u"read %d packets in time-shift file at packet index %d", {retcount, index});
    }
    return retcount;
}

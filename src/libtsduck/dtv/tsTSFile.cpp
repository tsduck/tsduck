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

#include "tsTSFile.h"
#include "tsTSPacketMetadata.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

// Maximum size of a packet header for non-TS format.
// Must be lower than the TS packet size to allow auto-detection on read.
namespace {
    constexpr size_t MAX_HEADER_SIZE = ts::TSPacketMetadata::SERIALIZATION_SIZE;
}

const ts::Enumeration ts::TSFile::FormatEnum({
    {u"autodetect", ts::TSFile::FMT_AUTODETECT},
    {u"TS",         ts::TSFile::FMT_TS},
    {u"M2TS",       ts::TSFile::FMT_M2TS},
    {u"duck",       ts::TSFile::FMT_DUCK},
});


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TSFile::TSFile() :
    _filename(),
    _total_read(0),
    _total_write(0),
    _repeat(0),
    _counter(0),
    _start_offset(0),
    _is_open(false),
    _flags(NONE),
    _severity(Severity::Error),
    _at_eof(false),
    _aborted(false),
    _rewindable(false),
    _regular(false),
    _format(FMT_AUTODETECT),
    _last_timestamp(0),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE)
#else
    _fd(-1)
#endif
{
}


//----------------------------------------------------------------------------
// Copy constructor.
//----------------------------------------------------------------------------

ts::TSFile::TSFile(const TSFile& other) :
    _filename(other._filename),
    _total_read(0),
    _total_write(0),
    _repeat(other._repeat),
    _counter(0),
    _start_offset(other._start_offset),
    _is_open(false),
    _flags(NONE),
    _severity(other._severity),
    _at_eof(false),
    _aborted(false),
    _rewindable(false),
    _regular(false),
    _format(other._format),
    _last_timestamp(other._last_timestamp),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE)
#else
    _fd(-1)
#endif
{
}


//----------------------------------------------------------------------------
// Move constructor.
//----------------------------------------------------------------------------

ts::TSFile::TSFile(TSFile&& other) noexcept :
    _filename(std::move(other._filename)),
    _total_read(other._total_read),
    _total_write(other._total_write),
    _repeat(other._repeat),
    _counter(other._counter),
    _start_offset(other._start_offset),
    _is_open(other._is_open),
    _flags(other._flags),
    _severity(other._severity),
    _at_eof(other._at_eof),
    _aborted(other._aborted),
    _rewindable(other._rewindable),
    _regular(other._regular),
    _format(other._format),
    _last_timestamp(other._last_timestamp),
#if defined(TS_WINDOWS)
    _handle(other._handle)
#else
    _fd(other._fd)
#endif
{
    // Mark other object as closed, just in case.
    other._is_open = false;
#if defined(TS_WINDOWS)
    other._handle = INVALID_HANDLE_VALUE;
#else
    other._fd = -1;
#endif
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TSFile::~TSFile()
{
    if (_is_open) {
        close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Get header size in bytes before a packet.
//----------------------------------------------------------------------------

size_t ts::TSFile::HeaderSize(Format format)
{
    switch (format) {
        case FMT_AUTODETECT: return 0;
        case FMT_TS:         return 0;
        case FMT_M2TS:       return 4;
        case FMT_DUCK:       return TSPacketMetadata::SERIALIZATION_SIZE;
        default:             return 0;
    }
}


//----------------------------------------------------------------------------
// Get the file name as a display string.
//----------------------------------------------------------------------------

ts::UString ts::TSFile::getDisplayFileName() const
{
    if (!_filename.empty()) {
        return _filename;
    }
    else if ((_flags & READ) != 0) {
        return u"standard input";
    }
    else if ((_flags & WRITE) != 0) {
        return u"standard output";
    }
    else {
        return u"closed";
    }
}


//----------------------------------------------------------------------------
// Open file for read in a rewindable mode.
//----------------------------------------------------------------------------

bool ts::TSFile::openRead(const UString& filename, uint64_t start_offset, Report& report, Format format)
{
    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }

    _filename = filename;
    _repeat = 1;
    _counter = 0;
    _start_offset = start_offset;
    _rewindable = true;
    _flags = READ;
    _format = format;

    return openInternal(false, report);
}


//----------------------------------------------------------------------------
// Open file for read with optional repetition.
//----------------------------------------------------------------------------

bool ts::TSFile::openRead(const UString& filename, size_t repeat_count, uint64_t start_offset, Report& report, Format format)
{
    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }

    _filename = filename;
    _repeat = repeat_count;
    _counter = 0;
    _start_offset = start_offset;
    _rewindable = false;
    _flags = READ | REOPEN_SPEC;
    _format = format;

    return openInternal(false, report);
}


//----------------------------------------------------------------------------
// Open file, generic form.
//----------------------------------------------------------------------------

bool ts::TSFile::open(const UString& filename, OpenFlags flags, Report& report, Format format)
{
    // Enforce WRITE if APPEND is specified.
    if ((flags & APPEND) != 0) {
        flags |= WRITE;
    }

    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }
    else if ((flags & (READ | WRITE)) == 0) {
        report.log(_severity, u"no read or write mode specified");
        return false;
    }
    else if (filename.empty() && (flags & READ) != 0 && (flags & WRITE) != 0) {
        report.log(_severity, u"cannot both read and write on standard input or output");
        return false;
    }

    _filename = filename;
    _repeat = 1;
    _counter = 0;
    _start_offset = 0;
    _rewindable = true;
    _flags = flags;
    _format = format;

    return openInternal(false, report);
}


//----------------------------------------------------------------------------
// Internal open
//----------------------------------------------------------------------------

bool ts::TSFile::openInternal(bool reopen, Report& report)
{
    const bool read_access = (_flags & READ) != 0;
    const bool write_access = (_flags & WRITE) != 0;
    const bool append_access = (_flags & APPEND) != 0;
    const bool read_only = (_flags & (READ | WRITE)) == READ;
    const bool keep_file = (_flags & KEEP) != 0;
    const bool temporary = (_flags & TEMPORARY) != 0;

    // Only named files can be reopened.
    if (reopen) {
        if (_filename.empty()) {
            report.log(_severity, u"internal error, cannot reopen standard input or output");
            return false;
        }
        else {
            report.debug(u"closing and reopening %s", {_filename});
        }
    }

#if defined(TS_WINDOWS)

    // Windows implementation
    const ::DWORD access = (read_access ? GENERIC_READ : 0) | (write_access ? GENERIC_WRITE : 0);
    const ::DWORD attrib = temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : FILE_ATTRIBUTE_NORMAL;
    const ::DWORD shared = read_only || (_flags & SHARE) != 0 ? FILE_SHARE_READ : 0;
    ::DWORD winflags = 0;

    // Close first if this is a reopen.
    if (reopen) {
        ::CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }

    if (read_only) {
        winflags = OPEN_EXISTING;
    }
    else if (read_access || append_access) {
        winflags = OPEN_ALWAYS;
    }
    else if (keep_file) {
        winflags = CREATE_NEW;
    }
    else {
        winflags = CREATE_ALWAYS;
    }

    if (_filename.empty()) {
        _handle = ::GetStdHandle(read_access ? STD_INPUT_HANDLE : STD_OUTPUT_HANDLE);
    }
    else {
        _handle = ::CreateFile(_filename.toUTF8().c_str(), access, shared, NULL, winflags, attrib, NULL);
        if (_handle == INVALID_HANDLE_VALUE) {
            const ErrorCode err = LastErrorCode();
            report.log(_severity, u"cannot open %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
            return false;
        }
        // Move to end of file if --append
        if (append_access && ::SetFilePointer(_handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
            const ErrorCode err = LastErrorCode();
            report.log(_severity, u"cannot append to %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
            ::CloseHandle(_handle);
            return false;
        }
    }

    // Check if this is a regular file.
    _regular = ::GetFileType(_handle) == FILE_TYPE_DISK;

    // Check if seek is required or possible.
    if (!seekCheck(report)) {
        if (!_filename.empty()) {
            ::CloseHandle(_handle);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0) {
        // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
        ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&_start_offset));
        if (::SetFilePointerEx(_handle, offset, NULL, FILE_BEGIN) == 0) {
            const ErrorCode err = LastErrorCode();
            report.log(_severity, u"error seeking file %s: %s", {_filename, ErrorCodeMessage(err)});
            if (!_filename.empty()) {
                ::CloseHandle(_handle);
            }
            return false;
        }
    }

#else

    // UNIX implementation
    int uflags = O_LARGEFILE;
    const mode_t mode = 0666; // -rw-rw-rw (minus umask)

    // Close first if this is a reopen.
    if (reopen) {
        ::close(_fd);
        _fd = -1;
    }

    if (read_only) {
        uflags |= O_RDONLY;
    }
    else if (!read_access) { // write only
        uflags |= O_WRONLY | O_CREAT;
        if (!append_access) {
            uflags |= O_TRUNC;
        }
    }
    else { // read + write
        uflags |= O_RDWR | O_CREAT;
    }
    if (write_access && keep_file) {
        uflags |= O_EXCL;
    }

    if (_filename.empty()) {
        // File name is empty means standard input or output. No need to open.
        _fd = read_access ? STDIN_FILENO : STDOUT_FILENO;
    }
    else {
        // Open a named file.
        if ((_fd = ::open(_filename.toUTF8().c_str(), uflags, mode)) < 0) {
            const ErrorCode err = LastErrorCode();
            report.log(_severity, u"cannot open file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
            return false;
        }
        // Move to end of file if --append.
        if (append_access && ::lseek(_fd, 0, SEEK_END) == off_t(-1)) {
            const ErrorCode err = LastErrorCode();
            report.log (_severity, u"error seeking at end of file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
            ::close(_fd);
            return false;
        }
        if (temporary) {
            // Immediately delete the file. It is removed from the directory.
            // It remains accessible as long as the file is open and is deleted on close.
            ::unlink(_filename.toUTF8().c_str());
        }
    }

    // Check if this is a regular file.
    struct stat st;
    if (::fstat(_fd, &st) < 0) {
        const ErrorCode err = LastErrorCode();
        report.log(_severity, u"cannot stat input file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
        if (!_filename.empty()) {
            ::close(_fd);
        }
        return false;
    }
    _regular = S_ISREG(st.st_mode);

    // Check if seek is required or possible.
    if (!seekCheck(report)) {
        if (!_filename.empty()) {
            ::close(_fd);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0 && ::lseek(_fd, off_t(_start_offset), SEEK_SET) == off_t(-1)) {
        const ErrorCode err = LastErrorCode();
        report.log (_severity, u"error seeking input file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
        if (!_filename.empty()) {
            ::close(_fd);
        }
        return false;
    }

#endif

    // Reset counters only if not a reopen.
    if (!reopen) {
        _total_read = _total_write = 0;
    }

    _last_timestamp = 0;
    _at_eof = _aborted = false;
    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Internal seek check. Return true when seeking is not required or possible.
// Return false if seeking is required but not possible.
//----------------------------------------------------------------------------

bool ts::TSFile::seekCheck(Report& report)
{
    if (_regular || (_repeat == 1 && _start_offset == 0)) {
        // Regular disk files can always be seeked.
        // Or no need to seek if the file is read only once, from the beginning.
        return true;
    }
    else if (_start_offset == 0 && !_filename.empty() && (_flags & (REOPEN | REOPEN_SPEC)) != 0) {
        // Force reopen at each rewind on non-regular named files when read from the beginning.
        _flags |= REOPEN;
        return true;
    }
    else {
        // We need to seek but we can't.
        report.log(_severity, u"input file %s is not a regular file, cannot %s", {getDisplayFileName(), _repeat != 1 ? u"repeat" : u"specify start offset"});
        return false;
    }
}


//----------------------------------------------------------------------------
// Internal seek. Rewind to specified start offset plus specified index.
//----------------------------------------------------------------------------

bool ts::TSFile::seekInternal(uint64_t index, Report& report)
{
    // If seeking at the beginning and REOPEN is set, close and reopen the file.
    if (index == 0 && (_flags & REOPEN) != 0) {
        return openInternal(true, report);
    }

    report.debug(u"seeking %s at offset %'d", {_filename, _start_offset + index});

#if defined(TS_WINDOWS)
    // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
    uint64_t where = _start_offset + index;
    ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&where));
    if (::SetFilePointerEx(_handle, offset, NULL, FILE_BEGIN) == 0) {
#else
    if (::lseek(_fd, off_t(_start_offset + index), SEEK_SET) == off_t(-1)) {
#endif
        const ErrorCode err = LastErrorCode();
        report.log(_severity, u"error seeking file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
        return false;
    }
    else {
        _at_eof = false;
        return true;
    }
}


//----------------------------------------------------------------------------
// Seek the file to the specified packet_index plus the start_offset.
//----------------------------------------------------------------------------

bool ts::TSFile::seek(PacketCounter packet_index, Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }
    else if (!_rewindable) {
        report.log(_severity, u"file %s is not rewindable", {getDisplayFileName()});
        return false;
    }
    else {
        return seekInternal(packet_index * (HeaderSize(_format) + PKT_SIZE), report);
    }
}


//----------------------------------------------------------------------------
// Close file.
//----------------------------------------------------------------------------

bool ts::TSFile::close(Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }

    if (!_filename.empty()) {
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
#else
        ::close(_fd);
#endif
    }

    _is_open = _at_eof = _aborted = false;
    _total_read = _total_write = 0;
    _flags = NONE;
    _filename.clear();

    return true;
}


//----------------------------------------------------------------------------
// Read internal implementation.
//----------------------------------------------------------------------------

bool ts::TSFile::readInternal(void* buffer, size_t request_size, size_t& read_size, Report& report)
{
    read_size = 0;
    char* data = reinterpret_cast<char*>(buffer);
    ErrorCode error_code = SYS_SUCCESS;
    bool success = true;

    // Loop on read until we get requested size or reach EOF or error.
    while (read_size < request_size && success && !_at_eof) {

#if defined(TS_WINDOWS)

        // Windows implementation
        ::DWORD insize = 0;
        if (::ReadFile(_handle, data, ::DWORD(request_size), &insize, NULL)) {
            // Normal case: some data were read
            assert(size_t(insize) <= request_size);
            data += size_t(insize);
            request_size -= size_t(insize);
            read_size += size_t(insize);
            _at_eof = _at_eof || insize == 0;
        }
        else {
            // Error case.
            error_code = LastErrorCode();
            _at_eof = _at_eof || error_code == ERROR_HANDLE_EOF || error_code == ERROR_BROKEN_PIPE;
            success = _at_eof; // reaching EOF is not an error.
        }

#else

        // UNIX implementation
        ssize_t insize = ::read(_fd, buffer, buffer_size);
        if (insize > 0) {
            // Normal case: some data were read
            assert(size_t(insize) <= request_size);
            data += size_t(insize);
            request_size -= size_t(insize);
            read_size += size_t(insize);
        }
        else if (insize == 0) {
            _at_eof = true;
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            success = false;
        }

#endif
    }

    if (!success) {
        report.log(_severity, u"error reading file %s: %s (%d)", {getDisplayFileName(), ErrorCodeMessage(error_code), error_code});
    }
    return success;
}


//----------------------------------------------------------------------------
// Read TS packets. Return the actual number of read packets.
//----------------------------------------------------------------------------

size_t ts::TSFile::read(TSPacket* buffer, size_t max_packets, Report& report, TSPacketMetadata* metadata)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return 0;
    }
    else if ((_flags & READ) == 0) {
        report.log(_severity, u"file %s is not open for read", {getDisplayFileName()});
        return 0;
    }
    else if (_aborted || _at_eof || max_packets == 0) {
        return 0;
    }

    // Number of read packets.
    size_t read_packets = 0;
    size_t read_size = 0;

    // Header buffer for M2TS or M8TS formats.
    uint8_t header[MAX_HEADER_SIZE];
    size_t header_size = HeaderSize(_format);
    assert(header_size <= sizeof(header));

    // If format is autodetect, read one packet to check where the sync byte is.
    if (_format == FMT_AUTODETECT) {

        // Read one packet.
        if (!readInternal(buffer, PKT_SIZE, read_size, report) || read_size < PKT_SIZE) {
            return 0; // less than one packet in that file
        }

        // Metadata for first packet (if there is a header).
        TSPacketMetadata mdata;

        // Check the position of the 0x47 sync byte to detect a potential header.
        if (buffer->b[0] == SYNC_BYTE) {
            // No header (or header starting with 0x47...)
            _format = FMT_TS;
        }
        else if (buffer->b[4] == SYNC_BYTE) {
            _format = FMT_M2TS;
            mdata.setInputTimeStamp(GetUInt32(buffer) & 0x3FFFFFFF, SYSTEM_CLOCK_FREQ);
        }
        else if (buffer->b[0] == TSPacketMetadata::SERIALIZATION_MAGIC && buffer->b[TSPacketMetadata::SERIALIZATION_SIZE] == SYNC_BYTE) {
            _format = FMT_DUCK;
            mdata.deserialize(buffer->b, TSPacketMetadata::SERIALIZATION_SIZE);
        }
        else {
            report.error(u"cannot detect format for TS file %s", {getDisplayFileName()});
            return 0;
        }
        report.debug(u"detected format %s for TS file %s", {getFormatString(), getDisplayFileName()});

        // If there was a header, remove it and read the rest of the packet.
        header_size = HeaderSize(_format);
        assert(header_size <= sizeof(header));
        if (header_size > 0) {
            // memmove() can move overlapping areas.
            char* data = reinterpret_cast<char*>(buffer);
            ::memmove(data, data + header_size, PKT_SIZE - header_size);
            if (!readInternal(data + PKT_SIZE - header_size, header_size, read_size, report) || read_size < header_size) {
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
    while (success && max_packets > 0 && !_at_eof) {

        switch (_format) {
            case FMT_AUTODETECT: {
                // Should not get there.
                assert(false);
                return 0;
            }
            case FMT_TS: {
                // Bulk read in TS format.
                success = readInternal(buffer, max_packets * PKT_SIZE, read_size, report);
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
            case FMT_M2TS:
            case FMT_DUCK: {
                // Read header + packet.
                success = readInternal(header, header_size, read_size, report);
                if (success && read_size == header_size) {
                    success = readInternal(buffer, PKT_SIZE, read_size, report);
                    if (success && read_size == PKT_SIZE) {
                        read_packets++;
                        buffer++;
                        max_packets--;
                        if (metadata != nullptr) {
                            if (_format == FMT_M2TS) {
                                metadata->reset();
                                metadata->setInputTimeStamp(GetUInt32(header) & 0x3FFFFFFF, SYSTEM_CLOCK_FREQ);
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
                report.error(u"invalid format %s for file %s", {getFormatString(), getDisplayFileName()});
                return 0;
            }
        }

        // At end of file, if the file must be repeated a finite number of times,
        // check if this was the last time. If the file must be repeated again,
        // rewind to original start offset.
        if (_at_eof && (_repeat == 0 || ++_counter < _repeat) && !seekInternal(0, report)) {
            return 0; // rewind error
        }
    }

    // Return the number of input packets.
    _total_read += read_packets;
    return read_packets;
}


//----------------------------------------------------------------------------
// Write internal implementation.
//----------------------------------------------------------------------------

bool ts::TSFile::writeInternal(const void* buffer, size_t data_size, size_t& written_size, Report& report)
{
    written_size = 0;
    ErrorCode error_code = SYS_SUCCESS;

#if defined(TS_WINDOWS)

    // Windows implementation
    const char* data = reinterpret_cast<const char*>(buffer);
    ::DWORD remain = ::DWORD(data_size);
    ::DWORD outsize = 0;

    // Loop on write until everything is gone
    while (remain > 0) {
        if (::WriteFile(_handle, data, remain, &outsize, NULL) != 0)  {
            // Normal case, some data were written
            outsize = std::min(outsize, remain);
            data += outsize;
            remain -= outsize;
            written_size += size_t(outsize);
        }
        else if ((error_code = LastErrorCode()) == ERROR_BROKEN_PIPE || error_code == ERROR_NO_DATA) {
            // Broken pipe: error state but don't report error.
            // Note that ERROR_NO_DATA (= 232) means "the pipe is being closed"
            // and this is the actual error code which is returned when the pipe
            // is closing, not ERROR_BROKEN_PIPE.
            return false;
        }
        else {
            // Write error
            report.log(_severity, u"error writing %s: %s (%d)", {getDisplayFileName(), ErrorCodeMessage(error_code), error_code});
            return false;
        }
    }
    return true;

#else

    // UNIX implementation
    const char* data = reinterpret_cast<const char*>(buffer);
    size_t remain = data_size;
    ssize_t outsize = 0;

    // Loop on write until everything is gone
    while (remain > 0) {
        outsize = ::write(_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            outsize = std::min<ssize_t>(outsize, remain);
            data += outsize;
            remain -= outsize;
            written_size += size_t(outsize);
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            // Don't report error on broken pipe.
            if (error_code != EPIPE) {
                report.log(_severity, u"error writing %s: %s (%d)", {getDisplayFileName(), ErrorCodeMessage(error_code), error_code});
            }
            return false;
        }
    }
    return true;

#endif
}


//----------------------------------------------------------------------------
// Write TS packets.
//----------------------------------------------------------------------------

bool ts::TSFile::write(const TSPacket* buffer, size_t packet_count, Report& report, const TSPacketMetadata* metadata)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }
    else if ((_flags & (WRITE | APPEND)) == 0) {
        report.log(_severity, u"file %s is not open for write", {getDisplayFileName()});
        return false;
    }
    else if (_aborted) {
        return false;
    }

    bool success = true;

    switch (_format) {
        case FMT_AUTODETECT:
        case FMT_TS: {
            // If file format is not yet known, force it as TS, the default.
            _format = FMT_TS;
            // Bulk write in TS format.
            size_t written_size = 0;
            success = writeInternal(buffer, packet_count * PKT_SIZE, written_size, report);
            _total_write += written_size / PKT_SIZE;
            break;
        }
        case FMT_M2TS:
        case FMT_DUCK: {
            // Write header + packet, packet by packet.
            uint8_t header[MAX_HEADER_SIZE];
            const size_t header_size = HeaderSize(_format);
            for (size_t i = 0; success && i < packet_count; ++i) {
                // Get time stamp of current packet or reuse last one.
                if (metadata != nullptr && metadata[i].hasInputTimeStamp()) {
                    _last_timestamp = metadata[i].getInputTimeStamp();
                }
                // Build header.
                if (_format == FMT_M2TS) {
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
                success = writeInternal(header, header_size, written_size, report) &&
                    writeInternal(&buffer[i], PKT_SIZE, written_size, report);
                if (success) {
                    _total_write++;
                }
            }
            break;
        }
        default: {
            report.error(u"invalid format %s for file %s", {getFormatString(), getDisplayFileName()});
            return false;
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Abort any currenly read/write operation in progress.
//----------------------------------------------------------------------------

void ts::TSFile::abort()
{
    if (_is_open) {
        // Mark broken pipe, read or write.
        _aborted = _at_eof = true;

        // Close pipe handle, ignore errors.
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
#else // UNIX
        ::close(_fd);
        _fd = -1;
#endif
    }
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFile.h"
#include "tsTSPacketMetadata.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <io.h>
    #include "tsAfterStandardHeaders.h"
#else
    #include "tsBeforeStandardHeaders.h"
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSFile::TSFile() :
    TSPacketStream(TSPacketFormat::AUTODETECT, this, this)
{
}

ts::TSFile::TSFile(TSFile&& other) noexcept :
    TSPacketStream(other.packetFormat(), this, this),
    _filename(std::move(other._filename)),
    _repeat(other._repeat),
    _counter(other._counter),
    _start_offset(other._start_offset),
    _open_null(other._open_null),
    _close_null(other._close_null),
    _open_null_read(other._open_null_read),
    _close_null_read(other._close_null_read),
    _is_open(other._is_open),
    _flags(other._flags),
    _severity(other._severity),
    _at_eof(other._at_eof),
    _aborted(other._aborted),
    _rewindable(other._rewindable),
    _regular(other._regular),
    _std_inout(other._std_inout),
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

ts::TSFile::~TSFile()
{
    if (_is_open) {
        close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Get the file name as a display string.
//----------------------------------------------------------------------------

ts::UString ts::TSFile::getDisplayFileName() const
{
    if (!_std_inout) {
        return UString(_filename);
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
// Set initial and final artificial stuffing.
//----------------------------------------------------------------------------

void ts::TSFile::setStuffing(size_t initial, size_t final)
{
    _open_null = initial;
    _close_null = final;
}


//----------------------------------------------------------------------------
// Open file for read in a rewindable mode.
//----------------------------------------------------------------------------

bool ts::TSFile::openRead(const fs::path& filename, uint64_t start_offset, Report& report, TSPacketFormat format)
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

    resetPacketStream(format, this, this);
    return openInternal(false, report);
}


//----------------------------------------------------------------------------
// Open file for read with optional repetition.
//----------------------------------------------------------------------------

bool ts::TSFile::openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset, Report& report, TSPacketFormat format)
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

    resetPacketStream(format, this, this);
    return openInternal(false, report);
}


//----------------------------------------------------------------------------
// Open file, generic form.
//----------------------------------------------------------------------------

bool ts::TSFile::open(const fs::path& filename, OpenFlags flags, Report& report, TSPacketFormat format)
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

    resetPacketStream(format, this, this);
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

    // Use standard input/output if file name is empty or a dash.
    _std_inout = _filename.empty() || _filename == u"-";

    // Only named files can be reopened.
    if (reopen) {
        if (_std_inout) {
            report.log(_severity, u"internal error, cannot reopen standard input or output");
            return false;
        }
        else {
            report.debug(u"closing and reopening %s", {_filename});
        }
    }

    // In read mode, preset the number of null packets to read.
    if (read_access && !reopen) {
        _open_null_read = _open_null;
        _close_null_read = _close_null;
    }

#if defined(TS_WINDOWS)

    // Windows implementation
    const ::DWORD access = (read_access ? GENERIC_READ : 0) | (write_access ? GENERIC_WRITE : 0);
    const ::DWORD attrib = temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : FILE_ATTRIBUTE_NORMAL;
    const ::DWORD shared = read_only || (_flags & SHARED) != 0 ? FILE_SHARE_READ : 0;
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

    if (!_std_inout) {
        // Actual file name, open it. On Windows, fs::path uses 16-bit wchar_t.
        _handle = ::CreateFileW(_filename.c_str(), access, shared, nullptr, winflags, attrib, nullptr);
        if (_handle == INVALID_HANDLE_VALUE) {
            const int err = LastSysErrorCode();
            report.log(_severity, u"cannot open %s: %s", {getDisplayFileName(), SysErrorCodeMessage(err)});
            return false;
        }
        // Move to end of file if --append
        if (append_access && ::SetFilePointer(_handle, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER) {
            const int err = LastSysErrorCode();
            report.log(_severity, u"cannot append to %s: %s", {getDisplayFileName(), SysErrorCodeMessage(err)});
            ::CloseHandle(_handle);
            return false;
        }
    }
    else if (read_access) {
        // Empty file name, read access, use standard input. Make it work in binary mode.
        _handle = ::GetStdHandle(STD_INPUT_HANDLE);
        ::_setmode(_fileno(stdin), _O_BINARY);
    }
    else {
        // Empty file name, write access, use standard output. Make it work in binary mode.
        _handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::_setmode(_fileno(stdout), _O_BINARY);
    }

    // Check if this is a regular file.
    _regular = ::GetFileType(_handle) == FILE_TYPE_DISK;

    // Check if seek is required or possible.
    if (!seekCheck(report)) {
        if (!_std_inout) {
            ::CloseHandle(_handle);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0) {
        // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
        ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&_start_offset));
        if (::SetFilePointerEx(_handle, offset, nullptr, FILE_BEGIN) == 0) {
            const int err = LastSysErrorCode();
            report.log(_severity, u"error seeking file %s: %s", {_filename, SysErrorCodeMessage(err)});
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

    if (_std_inout) {
        // File is standard input or output. No need to open.
        _fd = read_access ? STDIN_FILENO : STDOUT_FILENO;
    }
    else {
        // Open a named file.
        if ((_fd = ::open(_filename.c_str(), uflags, mode)) < 0) {
            report.log(_severity, u"cannot open file %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
            return false;
        }
        // Move to end of file if --append.
        if (append_access && ::lseek(_fd, 0, SEEK_END) == off_t(-1)) {
            report.log(_severity, u"error seeking at end of %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
            ::close(_fd);
            return false;
        }
        if (temporary) {
            // Immediately delete the file. It is removed from the directory.
            // It remains accessible as long as the file is open and is deleted on close.
            ::unlink(_filename.c_str());
        }
    }

    // Check if this is a regular file.
    struct stat st;
    if (::fstat(_fd, &st) < 0) {
        report.log(_severity, u"cannot stat input file %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }
    _regular = S_ISREG(st.st_mode);

    // Check if seek is required or possible.
    if (!seekCheck(report)) {
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0 && ::lseek(_fd, off_t(_start_offset), SEEK_SET) == off_t(-1)) {
        report.log(_severity, u"error seeking input file %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }

#endif

    // Reset counters only if not a reopen.
    if (!reopen) {
        _total_read = _total_write = 0;
    }

    // Clean initial state.
    _aborted = false;
    _at_eof = false;
    _is_open = true;

    // In write mode, write initial null packets.
    if (write_access && !reopen && _open_null > 0 && !writeStuffing(_open_null, report)) {
        close(report);
        return false;
    }

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
    else if (_start_offset == 0 && !_std_inout && (_flags & (REOPEN | REOPEN_SPEC)) != 0) {
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
    if (::SetFilePointerEx(_handle, offset, nullptr, FILE_BEGIN) == 0) {
#else
    if (::lseek(_fd, off_t(_start_offset + index), SEEK_SET) == off_t(-1)) {
#endif
        report.log(_severity, u"error seeking file %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
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
        return seekInternal(packet_index * (packetHeaderSize() + PKT_SIZE), report);
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

    // In write mode, write final null packets.
    if ((_flags & WRITE) != 0 && _close_null > 0) {
        writeStuffing(_close_null, report);
    }

    if (!_std_inout) {
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
#else
        ::close(_fd);
#endif
    }

    _is_open = false;
    _at_eof = false;
    _aborted = false;
    _flags = NONE;
    _filename.clear();
    _std_inout = false;

    return true;
}


//----------------------------------------------------------------------------
// Implementation of AbstractReadStreamInterface
//----------------------------------------------------------------------------

bool ts::TSFile::endOfStream()
{
    return _at_eof;
}

bool ts::TSFile::readStreamPartial(void* buffer, size_t request_size, size_t& read_size, Report& report)
{
    read_size = 0;

    if (!_is_open) {
        report.error(u"%s is not open", {getDisplayFileName()});
        return false;
    }
    if (_at_eof) {
        // Already at end of file. Do not report error.
        return false;
    }
    if (request_size == 0) {
        // Trivial case, successfully read zero bytes.
        return true;
    }

#if defined(TS_WINDOWS)

    // Windows implementation
    ::DWORD insize = 0;
    if (::ReadFile(_handle, buffer, ::DWORD(request_size), &insize, nullptr)) {
        // Normal case: some data were read
        assert(size_t(insize) <= request_size);
        read_size = size_t(insize);
        _at_eof = _at_eof || insize == 0;
        return read_size > 0;
    }
    else {
        // Error case.
        const int errcode = LastSysErrorCode();
        _at_eof = _at_eof || errcode == ERROR_HANDLE_EOF || errcode == ERROR_BROKEN_PIPE;
        if (!_at_eof) {
            // Actual error, not an EOF.
            report.error(u"error reading from %s: %s", {getDisplayFileName(), SysErrorCodeMessage(errcode)});
        }
        return false;
    }

#else

    // UNIX implementation
    for (;;) {
        const ssize_t insize = ::read(_fd, buffer, request_size);
        if (insize == 0) {
            // End of file.
            _at_eof = true;
            return false;
        }
        else if (insize > 0) {
            // Normal case, some data were read.
            assert(size_t(insize) <= request_size);
            read_size = size_t(insize);
            return true;
        }
        else if (errno != EINTR) {
            // Actual error (not an interrupt)
            report.log(_severity, u"error reading %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
            return false;
        }
    }

#endif
}


//----------------------------------------------------------------------------
// Read TS packets. Return the actual number of read packets.
// Override TSPacketStream implementation
//----------------------------------------------------------------------------

size_t ts::TSFile::readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets, Report& report)
{
    size_t ret_count = 0;

    // Initial artificial stuffing.
    if (_open_null_read > 0 && max_packets > 0) {
        const size_t count = std::min(max_packets, _open_null_read);
        report.debug(u"reading %d starting null packets", {count});
        readStuffing(buffer, metadata, count, report);
        _total_read += count;
        ret_count += count;
        max_packets -= count;
        _open_null_read -= count;
    }

    // Repeat reading packets until the buffer is full or error.
    // Rewind on end of file if repeating is set.
    while (max_packets > 0 && !_at_eof) {

        // Invoke superclass.
        const size_t count = TSPacketStream::readPackets(buffer, metadata, max_packets, report);

        if (count == 0 && !_at_eof) {
            break; // actual error
        }

        // Accumulate packets.
        ret_count += count;
        buffer += count;
        max_packets -= count;
        if (metadata != nullptr) {
            metadata += count;
        }

        // At end of file, if the file must be repeated a finite number of times,
        // check if this was the last time. If the file must be repeated again,
        // rewind to original start offset.
        if (_at_eof && (_repeat == 0 || ++_counter < _repeat) && !seekInternal(0, report)) {
            break; // rewind error
        }
    }

    // Final artificial stuffing.
    if (_at_eof && _close_null_read > 0 && max_packets > 0) {
        const size_t count = std::min(max_packets, _close_null_read);
        report.debug(u"reading %d stopping null packets", {count});
        readStuffing(buffer, metadata, count, report);
        _total_read += count;
        ret_count += count;
        _close_null_read -= count;
    }

    return ret_count;
}


//----------------------------------------------------------------------------
// Implementation of AbstractWriteStreamInterface
//----------------------------------------------------------------------------

bool ts::TSFile::writeStream(const void* buffer, size_t data_size, size_t& written_size, Report& report)
{
    written_size = 0;

#if defined(TS_WINDOWS)

    // Windows implementation
    const char* data = reinterpret_cast<const char*>(buffer);
    ::DWORD remain = ::DWORD(data_size);
    ::DWORD outsize = 0;
    ::DWORD errcode = ERROR_SUCCESS;

    // Loop on write until everything is gone
    while (remain > 0) {
        if (::WriteFile(_handle, data, remain, &outsize, nullptr) != 0)  {
            // Normal case, some data were written
            outsize = std::min(outsize, remain);
            data += outsize;
            remain -= outsize;
            written_size += size_t(outsize);
        }
        else if ((errcode = ::GetLastError()) == ERROR_BROKEN_PIPE || errcode == ERROR_NO_DATA) {
            // Broken pipe: error state but don't report error.
            // Note that ERROR_NO_DATA (= 232) means "the pipe is being closed"
            // and this is the actual error code which is returned when the pipe
            // is closing, not ERROR_BROKEN_PIPE.
            return false;
        }
        else {
            // Write error
            report.log(_severity, u"error writing %s: %s", {getDisplayFileName(), SysErrorCodeMessage(errcode)});
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
        else if (errno != EINTR) {
            // Actual error (not an interrupt). Don't report error on broken pipe.
            if (errno != EPIPE) {
                report.log(_severity, u"error writing %s: %s", {getDisplayFileName(), SysErrorCodeMessage()});
            }
            return false;
        }
    }
    return true;

#endif
}


//----------------------------------------------------------------------------
// Read/write artificial stuffing.
//----------------------------------------------------------------------------

void ts::TSFile::readStuffing(TSPacket*& buffer, TSPacketMetadata*& metadata, size_t count, Report& report)
{
    while (count-- > 0) {
        *buffer++ = NullPacket;
        if (metadata != nullptr) {
            (metadata++)->setInputStuffing(true);
        }
    }
}

bool ts::TSFile::writeStuffing(size_t count, Report& report)
{
    TSPacketMetadata mdata;
    mdata.setInputStuffing(true);
    for (size_t i = 0; i < count; ++i) {
        if (!writePackets(&NullPacket, &mdata, 1, report)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Abort any currenly read/write operation in progress.
//----------------------------------------------------------------------------

void ts::TSFile::abort()
{
    if (_is_open) {
        // Mark broken pipe, read or write.
        _aborted = true;
        _at_eof = true;

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

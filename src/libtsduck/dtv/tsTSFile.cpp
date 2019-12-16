//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsNullReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


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

bool ts::TSFile::openRead(const UString& filename, uint64_t start_offset, Report& report)
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

    return openInternal(report);
}


//----------------------------------------------------------------------------
// Open file for read with optional repetition.
//----------------------------------------------------------------------------

bool ts::TSFile::openRead(const UString& filename, size_t repeat_count, uint64_t start_offset, Report& report)
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
    _flags = READ;

    return openInternal(report);
}


//----------------------------------------------------------------------------
// Open file, generic form.
//----------------------------------------------------------------------------

bool ts::TSFile::open(const UString& filename, OpenFlags flags, Report& report)
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

    return openInternal(report);
}


//----------------------------------------------------------------------------
// Internal open
//----------------------------------------------------------------------------

bool ts::TSFile::openInternal(Report& report)
{
    const bool read_access = (_flags & READ) != 0;
    const bool write_access = (_flags & WRITE) != 0;
    const bool append_access = (_flags & APPEND) != 0;
    const bool read_only = (_flags & (READ | WRITE)) == READ;
    const bool keep_file = (_flags & KEEP) != 0;
    const bool temporary = (_flags & TEMPORARY) != 0;

#if defined(TS_WINDOWS)

    // Windows implementation
    const ::DWORD access = (read_access ? GENERIC_READ : 0) | (write_access ? GENERIC_WRITE : 0);
    const ::DWORD attrib = temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : FILE_ATTRIBUTE_NORMAL;
    const ::DWORD shared = read_only || (_flags & SHARE) != 0 ? FILE_SHARE_READ : 0;
    ::DWORD winflags = 0;

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

    // If a repeat count or initial offset is specified, the input file must be a regular file
    if ((_repeat != 1 || _start_offset != 0) && ::GetFileType(_handle) != FILE_TYPE_DISK) {
        report.log(_severity, u"file %s is not a regular file, cannot %s", {getDisplayFileName(), _repeat != 1 ? u"repeat" : u"specify start offset"});
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

    if (read_only) {
        uflags |= O_RDONLY;
    }
    else if (!read_access) { // write only
        uflags |= O_WRONLY | O_CREAT | O_TRUNC;
    }
    else { // read + write
        uflags |= O_RDWR | O_CREAT;
    }
    if (append_access) {
        uflags |= O_APPEND;
    }
    if (write_access && keep_file) {
        uflags |= O_EXCL;
    }
    
    if (_filename.empty()) {
        _fd = read_access ? STDIN_FILENO : STDOUT_FILENO;
    }
    else if ((_fd = ::open(_filename.toUTF8().c_str(), uflags, mode)) < 0) {
        const ErrorCode err = LastErrorCode();
        report.log(_severity, u"cannot open file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
        return false;
    }
    else if (temporary) {
        // Immediately delete the file. It is removed from the directory.
        // It remains accessible as long as the file is open and is deleted on close.
        ::unlink(_filename.toUTF8().c_str());
    }

    // If a repeat count or initial offset is specified, the input file must be a regular file
    if (_repeat != 1 || _start_offset != 0) {
        struct stat st;
        if (::fstat(_fd, &st) < 0) {
            const ErrorCode err = LastErrorCode();
            report.log(_severity, u"cannot stat input file %s: %s", {getDisplayFileName(), ErrorCodeMessage(err)});
            if (!_filename.empty()) {
                ::close(_fd);
            }
            return false;
        }
        if (!S_ISREG(st.st_mode)) {
            report.log(_severity, u"input file %s is not a regular file, cannot %s", {getDisplayFileName(), _repeat != 1 ? u"repeat" : u"specify start offset"});
            if (!_filename.empty()) {
                ::close(_fd);
            }
            return false;
        }
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

    _total_read = _total_write = 0;
    _at_eof = _aborted = false;
    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Internal seek. Rewind to specified start offset plus specified index.
//----------------------------------------------------------------------------

bool ts::TSFile::seekInternal(uint64_t index, Report& report)
{
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
        return seekInternal(packet_index * PKT_SIZE, report);
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
// Read TS packets. Return the actual number of read packets.
//----------------------------------------------------------------------------

size_t ts::TSFile::read(TSPacket* buffer, size_t max_packets, Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return 0;
    }
    else if ((_flags & READ) == 0) {
        report.log(_severity, u"file %s is not open for read", {getDisplayFileName()});
        return 0;
    }
    else if (_aborted || _at_eof) {
        return 0;
    }

    char* data = reinterpret_cast<char*>(buffer);
    const size_t req_size = max_packets * PKT_SIZE;
    size_t got_size = 0;
    bool got_error = false;
    ErrorCode error_code = 0;

    // Loop on read until we get enough
    while (got_size < req_size && !_at_eof && !got_error) {

#if defined(TS_WINDOWS)
        // Windows implementation
        ::DWORD insize;
        if (::ReadFile(_handle, data + got_size, ::DWORD (req_size - got_size), &insize, NULL)) {
            // Normal case: some data were read
            got_size += insize;
            assert(got_size <= req_size);
            _at_eof = _at_eof || insize == 0;
        }
        else {
            error_code = LastErrorCode ();
            _at_eof = _at_eof || error_code == ERROR_HANDLE_EOF || error_code == ERROR_BROKEN_PIPE;
            got_error = !_at_eof;
        }
#else
        // UNIX implementation
        ssize_t insize = ::read(_fd, data + got_size, req_size - got_size);
        if (insize > 0) {
            // Normal case: some data were read
            got_size += insize;
            assert(got_size <= req_size);
        }
        else if (insize == 0) {
            _at_eof = true;
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            got_error = true;
        }
#endif

        // At end-of-file, truncate partial packet.
        if (_at_eof) {
            got_size -= got_size % PKT_SIZE;
        }

        // At end of file, if the file must be repeated a finite number of times,
        // check if this was the last time. If the file must be repeated again,
        // rewind to original start offset.
        if (_at_eof && (_repeat == 0 || ++_counter < _repeat) && !seekInternal (0, report)) {
            return 0; // rewind error
        }
    }

    if (got_error) {
        report.log(_severity, u"error reading file %s: %s (%d)", {_filename, ErrorCodeMessage(error_code), error_code});
        return 0;
    }

    // Return the number of input packets.
    const size_t count = got_size / PKT_SIZE;
    _total_read += count;
    return count;
}


//----------------------------------------------------------------------------
// Write method
//----------------------------------------------------------------------------

bool ts::TSFile::write(const TSPacket* buffer, size_t packet_count, Report& report)
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

    // Loop on write until everything is gone
    bool got_error = false;
    ErrorCode error_code = SYS_SUCCESS;
    const char* const data_buffer = reinterpret_cast<const char*>(buffer);
    const char* data = data_buffer;

#if defined(TS_WINDOWS)

    // Windows implementation
    ::DWORD remain = ::DWORD(packet_count) * PKT_SIZE;
    ::DWORD outsize;

    while (remain > 0 && !got_error) {
        if (::WriteFile(_handle, data, remain, &outsize, NULL) != 0)  {
            // Normal case, some data were written
            outsize = std::min(outsize, remain);
            data += outsize;
            remain -= std::max(remain, outsize);
        }
        else if ((error_code = LastErrorCode()) == ERROR_BROKEN_PIPE || error_code == ERROR_NO_DATA) {
            // Broken pipe: error state but don't report error.
            // Note that ERROR_NO_DATA (= 232) means "the pipe is being closed"
            // and this is the actual error code which is returned when the pipe
            // is closing, not ERROR_BROKEN_PIPE.
            error_code = SYS_SUCCESS;
            got_error = true;
        }
        else {
            // Write error
            got_error = true;
        }
    }

#else

    // UNIX implementation
    size_t remain = packet_count * PKT_SIZE;
    ssize_t outsize = 0;

    while (remain > 0 && !got_error) {
        outsize = ::write(_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            assert(size_t(outsize) <= remain);
            data += outsize;
            remain -= std::max(remain, size_t (outsize));
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            report.debug(u"write error on %s, fd=%d, error_code=%d", {getDisplayFileName(), _fd, error_code});
            got_error = true;
            if (error_code == EPIPE) {
                // Broken pipe: keep the error state but don't report error.
                error_code = SYS_SUCCESS;
            }
        }
    }

#endif

    if (got_error && error_code != SYS_SUCCESS) {
        report.log(_severity, u"error writing %s: %s (%d)", {getDisplayFileName(), ErrorCodeMessage(error_code), error_code});
    }

    _total_write += (data - data_buffer) / PKT_SIZE;
    return !got_error;
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

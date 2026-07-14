//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBinaryFile.h"
#include "tsSysUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <io.h>
    #include "tsAfterStandardHeaders.h"
    #include "tsWinUtils.h"
#else
    #include "tsBeforeStandardHeaders.h"
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::BinaryFile::BinaryFile(Report* report, bool non_blocking, Object* owner) :
    NonBlockingDevice(report, non_blocking, owner),
    AbstractStream(*static_cast<ReporterBase*>(this))
{
}

ts::BinaryFile::BinaryFile(ReporterBase* delegate, bool non_blocking, Object* owner) :
    NonBlockingDevice(delegate, non_blocking, owner),
    AbstractStream(*static_cast<ReporterBase*>(this))
{
}

ts::BinaryFile::~BinaryFile()
{
    if (_is_open) {
        BinaryFile::close(true);
    }
}


//----------------------------------------------------------------------------
// Get the file name as a display string.
//----------------------------------------------------------------------------

ts::UString ts::BinaryFile::getDisplayFileName() const
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
// Open file for read in a rewindable mode.
//----------------------------------------------------------------------------

bool ts::BinaryFile::openRead(const fs::path& filename, uint64_t start_offset)
{
    if (_is_open) {
        report().error(u"already open");
        return false;
    }

    _filename = filename;
    _repeat_input = 1;
    _input_counter = 0;
    _start_offset = start_offset;
    _rewindable = true;
    _flags = READ;

    return openInternal(false);
}


//----------------------------------------------------------------------------
// Open file for read with optional repetition.
//----------------------------------------------------------------------------

bool ts::BinaryFile::openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset)
{
    if (_is_open) {
        report().error(u"already open");
        return false;
    }

    _filename = filename;
    _repeat_input = repeat_count;
    _input_counter = 0;
    _start_offset = start_offset;
    _rewindable = false;
    _flags = READ | REOPEN_SPEC;

    return openInternal(false);
}


//----------------------------------------------------------------------------
// Open file, generic form.
//----------------------------------------------------------------------------

bool ts::BinaryFile::open(const fs::path& filename, OpenFlags flags)
{
    if (_is_open) {
        report().error(u"already open");
        return false;
    }

    _filename = filename;
    _repeat_input = 1;
    _input_counter = 0;
    _start_offset = 0;
    _rewindable = true;
    _flags = flags;

    if ((_flags & APPEND) != 0) {
        // Enforce WRITE if APPEND is specified.
        _flags |= WRITE;
    }

    return openInternal(false);
}


//----------------------------------------------------------------------------
// Internal open
//----------------------------------------------------------------------------

bool ts::BinaryFile::openInternal(bool reopen)
{
    const bool read_access = (_flags & READ) != 0;
    const bool write_access = (_flags & WRITE) != 0;
    const bool append_access = (_flags & APPEND) != 0;
    const bool read_only = (_flags & (READ | WRITE)) == READ;
    const bool keep_file = (_flags & KEEP) != 0;
    const bool temporary = (_flags & TEMPORARY) != 0;

    // Use standard input/output if file name is empty or a dash.
    _std_inout = _filename.empty() || _filename == u"-";
    if (_std_inout) {
        // Normalize standard input/output to empty string.
        _filename.clear();
    }

    if ((_flags & (READ | WRITE)) == 0) {
        report().error(u"no read or write mode specified");
        return false;
    }
    else if (_std_inout && (_flags & READ) != 0 && (_flags & WRITE) != 0) {
        report().error(u"cannot both read and write on standard input or output");
        return false;
    }

    // Only named files can be reopened.
    if (reopen) {
        if (_std_inout) {
            report().error(u"internal error, cannot reopen standard input or output");
            return false;
        }
        else {
            report().debug(u"closing and reopening %s", _filename);
        }
    }

#if defined(TS_WINDOWS)

    //@@@ Asynchronous I/O

    // Windows implementation
    const ::DWORD access_mask = (read_access ? GENERIC_READ : 0) | (write_access ? GENERIC_WRITE : 0);
    const ::DWORD attrib = temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : FILE_ATTRIBUTE_NORMAL;
    const ::DWORD shared = read_only || (_flags & SHARED) != 0 ? FILE_SHARE_READ : 0;
    ::DWORD winflags = 0;

    // Close first if this is a reopen.
    if (reopen) {
        ::CloseHandle(_handle);
        _handle = nullptr;
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
        _handle = ::CreateFileW(_filename.c_str(), access_mask, shared, nullptr, winflags, attrib, nullptr);
        if (!WinHandleValid(_handle)) {
            const int err = LastSysErrorCode();
            report().error(u"cannot open %s: %s", getDisplayFileName(), SysErrorCodeMessage(err));
            return false;
        }
        // Move to end of file if --append
        if (append_access && ::SetFilePointer(_handle, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER) {
            const int err = LastSysErrorCode();
            report().error(u"cannot append to %s: %s", getDisplayFileName(), SysErrorCodeMessage(err));
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
    if (!initialSeekCheck()) {
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
            report().error(u"error seeking file %s: %s", _filename, SysErrorCodeMessage(err));
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
            report().error(u"cannot open file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
            return false;
        }
        // Move to end of file if --append.
        if (append_access && ::lseek(_fd, 0, SEEK_END) == off_t(-1)) {
            report().error(u"error seeking at end of %s: %s", getDisplayFileName(), SysErrorCodeMessage());
            ::close(_fd);
            return false;
        }
        if (temporary) {
            // Immediately delete the file. It is removed from the directory.
            // It remains accessible as long as the file is open and is deleted on close.
            ::unlink(_filename.c_str());
        }
    }

    // Set the file descriptor in non-blocking mode if necessary.
    if (isNonBlocking() && !setSystemNonBlocking(_fd, true)) {
        ::close(_fd);
        return false;
    }

    // Check if this is a regular file.
    struct stat st {};
    if (::fstat(_fd, &st) < 0) {
        report().error(u"cannot stat input file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }
    _regular = S_ISREG(st.st_mode);

    // Check if seek is required or possible.
    if (!initialSeekCheck()) {
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0 && ::lseek(_fd, off_t(_start_offset), SEEK_SET) == off_t(-1)) {
        report().error(u"error seeking input file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
        if (!_std_inout) {
            ::close(_fd);
        }
        return false;
    }

#endif

    // Clean initial state.
    _aborted = false;
    _at_eof = false;
    _is_open = true;

    return true;
}


//----------------------------------------------------------------------------
// Seek check: called during open to see if we can reach the start point.
//----------------------------------------------------------------------------

bool ts::BinaryFile::initialSeekCheck()
{
    if (_regular || (_repeat_input == 1 && _start_offset == 0)) {
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
        report().error(u"input file %s is not a regular file, cannot %s", getDisplayFileName(), _repeat_input != 1 ? u"repeat" : u"specify start offset");
        return false;
    }
}


//----------------------------------------------------------------------------
// Rewind to specified start offset plus specified index.
//----------------------------------------------------------------------------

bool ts::BinaryFile::seekByte(uint64_t index)
{
    if (!_is_open) {
        report().error(u"not open");
        return false;
    }
    else if (!_rewindable) {
        report().error(u"file %s is not rewindable", getDisplayFileName());
        return false;
    }

    // If seeking at the beginning and REOPEN is set, close and reopen the file.
    if (index == 0 && (_flags & REOPEN) != 0) {
        return openInternal(true);
    }

    report().debug(u"seeking %s at offset %'d", _filename, _start_offset + index);

#if defined(TS_WINDOWS)
    // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
    uint64_t where = _start_offset + index;
    ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&where));
    if (::SetFilePointerEx(_handle, offset, nullptr, FILE_BEGIN) == 0) {
#else
    if (::lseek(_fd, off_t(_start_offset + index), SEEK_SET) == off_t(-1)) {
#endif
        report().error(u"error seeking file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
        return false;
    }
    else {
        _at_eof = false;
        return true;
    }
}


//----------------------------------------------------------------------------
// Close file.
//----------------------------------------------------------------------------

bool ts::BinaryFile::close(bool silent)
{
    if (!_is_open) {
        report().log(SilentLevel(silent), u"not open");
        return false;
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
// Implementation of AbstractStream (read).
//----------------------------------------------------------------------------

bool ts::BinaryFile::endOfStream()
{
    // Return an end-of-file at end of the last iteration.
    return _at_eof && _repeat_input != 0 && _input_counter >= _repeat_input;
}

bool ts::BinaryFile::readStream(void* buffer, size_t request_size, size_t& read_size, const AbortInterface* abort, IOSB* iosb)
{
    // Clear returned size.
    read_size = 0;

    if (!checkNonBlocking(iosb, u"BinaryFile::readStream")) {
        // Not the right blocking mode.
        return false;
    }
    if (!_is_open) {
        report().error(u"%s is not open", getDisplayFileName());
        return false;
    }
    if (BinaryFile::endOfStream()) {
        // Already at end of file, all iterations included. Do not report error.
        return false;
    }
    if (request_size == 0) {
        // Trivial case, successfully read zero bytes.
        return true;
    }
    if (_at_eof) {
        // We are at eof for this iteration, but more are allowed. Rewind the file.
        if (!rewind()) {
            return false;
        }
        _input_counter++;
    }

#if defined(TS_WINDOWS)

    //@@@ Asynchronous I/O

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
            report().error(u"error reading from %s: %s", getDisplayFileName(), SysErrorCodeMessage(errcode));
        }
        return false;
    }

#else

    // UNIX implementation
    for (;;) {
        const ssize_t insize = ::read(_fd, buffer, request_size);
        const int err_code = errno;
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
        else if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
        else if (isNonBlocking() && IsPendingStatus(err_code)) {
            // Non-blocking file with no immediately available data to read.
            assert(iosb != nullptr);
            iosb->pending = true;
            return true;
        }
        else if (err_code != EINTR) {
            // Actual error (not an interrupt)
            report().error(u"error reading %s: %s", getDisplayFileName(), SysErrorCodeMessage(err_code));
            return false;
        }
    }

#endif
}


//----------------------------------------------------------------------------
// Implementation of AbstractStream (write).
//----------------------------------------------------------------------------

bool ts::BinaryFile::writeStream(const void* buffer, size_t data_size, size_t& written_size, IOSB* iosb)
{
    // Clear returned size.
    written_size = 0;

    if (!checkNonBlocking(iosb, u"BinaryFile::writeStream")) {
        // Not the right blocking mode.
        return false;
    }

#if defined(TS_WINDOWS)

    //@@@ Asynchronous I/O

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
            report().error(u"error writing %s: %s", getDisplayFileName(), SysErrorCodeMessage(errcode));
            return false;
        }
    }
    return true;

#else

    // UNIX implementation
    const char* data = reinterpret_cast<const char*>(buffer);
    size_t remain = data_size;

    // Loop on write until everything is gone
    while (remain > 0) {
        ssize_t outsize = ::write(_fd, data, remain);
        const int err_code = errno;
        if (outsize > 0) {
            // Normal case, some data were written
            outsize = std::min<ssize_t>(outsize, remain);
            data += outsize;
            remain -= outsize;
            written_size += size_t(outsize);
        }
        else if (isNonBlocking() && IsPendingStatus(err_code)) {
            // Non-blocking file with no enough available outgoing buffer space.
            assert(iosb != nullptr);
            iosb->pending = true;
            iosb->sent_size = written_size;
            return true;
        }
        else if (err_code != EINTR) {
            // Actual error (not an interrupt). Don't report error on broken pipe.
            if (err_code != EPIPE) {
                report().error(u"error writing %s: %s", getDisplayFileName(), SysErrorCodeMessage(err_code));
            }
            return false;
        }
    }
    return true;

#endif
}


//----------------------------------------------------------------------------
// Abort any currenly read/write operation in progress.
//----------------------------------------------------------------------------

void ts::BinaryFile::abort()
{
    if (_is_open) {
        // Mark broken pipe, read or write.
        _aborted = true;
        _at_eof = true;

        // Close pipe handle, ignore errors.
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
        _handle = nullptr;
#else // UNIX
        ::close(_fd);
        _fd = -1;
#endif
    }
}

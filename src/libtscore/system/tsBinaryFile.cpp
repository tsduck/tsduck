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
    NonBlockingDevice(report, non_blocking, owner)
{
}

ts::BinaryFile::BinaryFile(ReporterBase* delegate, bool non_blocking, Object* owner) :
    NonBlockingDevice(delegate, non_blocking, owner)
{
}

ts::BinaryFile::~BinaryFile()
{
    if (_is_open) {
        BinaryFile::close(true);
    }
}


//----------------------------------------------------------------------------
// Check that the non-blocking mode can be set.
//----------------------------------------------------------------------------

bool ts::BinaryFile::allowSetNonBlocking() const
{
    // Cannot change the blocking mode after open.
    return !isOpen();
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
            #if defined(TS_WINDOWS)
                ::CloseHandle(_hfd);
            #else
                ::close(_hfd);
            #endif
            _hfd = SYS_HANDLE_INVALID;
        }
    }

#if defined(TS_WINDOWS)

    // Windows implementation.
    const ::DWORD access_mode = (read_access ? GENERIC_READ : 0) | (write_access ? GENERIC_WRITE : 0);
    const ::DWORD share_mode = read_only || (_flags & SHARED) != 0 ? FILE_SHARE_READ : 0;
    const ::DWORD flags_attr = (temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : FILE_ATTRIBUTE_NORMAL) | (isNonBlocking() ? FILE_FLAG_OVERLAPPED : 0);

    ::DWORD create_flags = 0;
    if (read_only) {
        create_flags = OPEN_EXISTING;
    }
    else if (read_access || append_access) {
        create_flags = OPEN_ALWAYS;
    }
    else if (keep_file) {
        create_flags = CREATE_NEW;
    }
    else {
        create_flags = CREATE_ALWAYS;
    }

    if (_std_inout) {
        // Use stdin or stdout.
        if (read_access) {
            // Read access => use standard input. Make it work in binary mode.
            _hfd = ::GetStdHandle(STD_INPUT_HANDLE);
            ::_setmode(_fileno(stdin), _O_BINARY);
        }
        else {
            // Write access => use standard output. Make it work in binary mode.
            _hfd = ::GetStdHandle(STD_OUTPUT_HANDLE);
            ::_setmode(_fileno(stdout), _O_BINARY);
        }
        // In the general case, Windows doesn't support setting FILE_FLAG_OVERLAPPED on existing
        // handles, such as stdin or stdout. Additionally, some devices such as the console or
        // anonymous pipes don't support FILE_FLAG_OVERLAPPED at all. If stdin or stdout is a
        // file, we can try to reopen. Otherwise, it will fail.
        if (isNonBlocking()) {
            const ::DWORD ftype = ::GetFileType(_hfd);
            if (ftype == FILE_TYPE_DISK || ftype == FILE_TYPE_PIPE) {
                // Try to reopen with FILE_FLAG_OVERLAPPED. Will work on disk files and named pipes.
                // Will fail on anonymous pipes (but we cannot distinguish between the two types of pipes).
                _hfd = ::ReOpenFile(_hfd, access_mode, 0, FILE_FLAG_OVERLAPPED);
                if (!WinHandleValid(_hfd)) {
                    const int err = LastSysErrorCode();
                    report().error(u"failed to reopen %s in overlapped mode: %s", getDisplayFileName(), SysErrorCodeMessage(err));
                    return false;
                }
            }
            else {
                report().error(u"Windows does not support asynchronous I/O on standard input/output");
                return false;
            }
        }
    }
    else {
        // Open or create a named file. On Windows, fs::path uses 16-bit wchar_t.
        _hfd = ::CreateFileW(_filename.c_str(), access_mode, share_mode, nullptr, create_flags, flags_attr, nullptr);
        if (!WinHandleValid(_hfd)) {
            const int err = LastSysErrorCode();
            report().error(u"cannot open %s: %s", getDisplayFileName(), SysErrorCodeMessage(err));
            return false;
        }
        // Move to end of file if append mode.
        if (append_access && ::SetFilePointer(_hfd, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER) {
            const int err = LastSysErrorCode();
            report().error(u"cannot append to %s: %s", getDisplayFileName(), SysErrorCodeMessage(err));
            ::CloseHandle(_hfd);
            _hfd = SYS_HANDLE_INVALID;
            return false;
        }
    }

    // Check if this is a regular file.
    _regular = ::GetFileType(_hfd) == FILE_TYPE_DISK;

    // Check if seek is required or possible.
    if (!initialSeekCheck()) {
        if (!_std_inout) {
            ::CloseHandle(_hfd);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0) {
        // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
        ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&_start_offset));
        if (::SetFilePointerEx(_hfd, offset, nullptr, FILE_BEGIN) == 0) {
            const int err = LastSysErrorCode();
            report().error(u"error seeking file %s: %s", _filename, SysErrorCodeMessage(err));
            if (!_filename.empty()) {
                ::CloseHandle(_hfd);
            }
            return false;
        }
    }

#else

    // UNIX implementation.
    int uflags = O_LARGEFILE;
    const mode_t mode = 0666;  // -rw-rw-rw (minus umask)

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
        _hfd = read_access ? STDIN_FILENO : STDOUT_FILENO;
    }
    else {
        // Open a named file.
        if ((_hfd = ::open(_filename.c_str(), uflags, mode)) < 0) {
            report().error(u"cannot open file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
            return false;
        }
        // Move to end of file if append mode.
        if (append_access && ::lseek(_hfd, 0, SEEK_END) == off_t(-1)) {
            report().error(u"error seeking at end of %s: %s", getDisplayFileName(), SysErrorCodeMessage());
            ::close(_hfd);
            return false;
        }
        if (temporary) {
            // Immediately delete the file. It is removed from the directory.
            // It remains accessible as long as the file is open and is deleted on close.
            ::unlink(_filename.c_str());
        }
    }

    // Set the file descriptor in non-blocking mode if necessary.
    if (isNonBlocking() && !setSystemNonBlocking(_hfd, true)) {
        ::close(_hfd);
        return false;
    }

    // Check if this is a regular file.
    struct stat st {};
    if (::fstat(_hfd, &st) < 0) {
        report().error(u"cannot stat input file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
        if (!_std_inout) {
            ::close(_hfd);
        }
        return false;
    }
    _regular = S_ISREG(st.st_mode);

    // Check if seek is required or possible.
    if (!initialSeekCheck()) {
        if (!_std_inout) {
            ::close(_hfd);
        }
        return false;
    }

    // If an initial offset is specified, move here
    if (_start_offset != 0 && ::lseek(_hfd, off_t(_start_offset), SEEK_SET) == off_t(-1)) {
        report().error(u"error seeking input file %s: %s", getDisplayFileName(), SysErrorCodeMessage());
        if (!_std_inout) {
            ::close(_hfd);
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

// Public version, check that the file rewindable.
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
    else {
        return seekByteInternal(index);
    }
}

// Internal version, technical seek.
bool ts::BinaryFile::seekByteInternal(uint64_t index)
{
    // If seeking at the beginning and REOPEN is set, close and reopen the file.
    if (index == 0 && (_flags & REOPEN) != 0) {
        return openInternal(true);
    }

    report().debug(u"seeking %s at offset %'d", _filename, _start_offset + index);

#if defined(TS_WINDOWS)
    // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
    uint64_t where = _start_offset + index;
    ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&where));
    if (::SetFilePointerEx(_hfd, offset, nullptr, FILE_BEGIN) == 0) {
#else
    if (::lseek(_hfd, off_t(_start_offset + index), SEEK_SET) == off_t(-1)) {
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
        ::CloseHandle(_hfd);
#else
        ::close(_hfd);
#endif
    }

    _is_open = false;
    _at_eof = false;
    _aborted = false;
    _flags = NONE;
    _filename.clear();
    _std_inout = false;
    _hfd = SYS_HANDLE_INVALID;

    return true;
}


//----------------------------------------------------------------------------
// Implementation of StreamInterface (read).
//----------------------------------------------------------------------------

bool ts::BinaryFile::endOfStream()
{
    // Return an end-of-file at end of the last iteration.
    return _at_eof && _repeat_input != 0 && _input_counter >= _repeat_input;
}

bool ts::BinaryFile::readStream(void* buffer, size_t size, const AbortInterface* abort)
{
    return ReadStreamHelper<BinaryFile>(this, buffer, size, abort);
}

bool ts::BinaryFile::readStream(void* buffer, size_t request_size, size_t& read_size, const AbortInterface* abort, IOSB* iosb)
{
    read_size = 0;

    if (!_is_open) {
        report().error(u"%s is not open", getDisplayFileName());
        return false;
    }
    if (BinaryFile::endOfStream()) {
        // Already at end of file, all iterations included. Do not report error.
        return false;
    }

    // Try to read once. Perform the read using generic code.
    int err_code = genericSystemRead(_hfd, buffer, request_size, read_size, abort, iosb);
    _at_eof = _at_eof || err_code == SYS_EOF;

    // If we are at eof, rewind and retry.
    if (err_code == SYS_EOF && (++_input_counter < _repeat_input || _repeat_input == 0)) {
        report().debug(u"rewind file %s, start iteration %d/%d", _filename, _input_counter, _repeat_input);
        if (!seekByteInternal(0)) {
            return false;
        }
        err_code = genericSystemRead(_hfd, buffer, request_size, read_size, abort, iosb);
        _at_eof = _at_eof || err_code == SYS_EOF;
    }
    return SysSuccess(err_code);
}


//----------------------------------------------------------------------------
// Implementation of StreamInterface (write).
//----------------------------------------------------------------------------

bool ts::BinaryFile::writeStream(const void* buffer, size_t data_size, IOSB* iosb)
{
    return WriteStreamHelper<BinaryFile>(this, buffer, data_size, iosb);
}

bool ts::BinaryFile::writeStream(const void* buffer, size_t data_size, size_t& written_size, IOSB* iosb)
{
    // Perform the write using generic code.
    const int err_code = genericSystemWrite(_hfd, buffer, data_size, written_size, iosb);
    return SysSuccess(err_code);
}


//----------------------------------------------------------------------------
// Abort any currenly read/write operation in progress.
//----------------------------------------------------------------------------

void ts::BinaryFile::abort()
{
    if (_is_open) {
        // Mark broken file, read or write.
        _aborted = true;
        _at_eof = true;

        // Close file handle, ignore errors.
#if defined(TS_WINDOWS)
        ::CloseHandle(_hfd);
#else // UNIX
        ::close(_hfd);
#endif
        _hfd = SYS_HANDLE_INVALID;
    }
}

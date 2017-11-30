//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Transport stream file input
//
//----------------------------------------------------------------------------

#include "tsTSFileInput.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TSFileInput::TSFileInput() :
    _filename(),
    _total_packets(0),
    _repeat(0),
    _counter(0),
    _start_offset(0),
    _is_open(false),
    _severity(Severity::Error),
    _at_eof(false),
    _rewindable(false),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE)
#else
    _fd(-1)
#endif
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TSFileInput::~TSFileInput()
{
    if (_is_open) {
        close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Open file in a rewindable mode (must be a rewindable file, eg. not a pipe).
//----------------------------------------------------------------------------

bool ts::TSFileInput::open(const UString& filename, uint64_t start_offset, Report& report)
{
    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }

    _filename = filename;
    _repeat = 1;
    _counter = 0;
    _start_offset = start_offset;
    _at_eof = false;
    _rewindable = true;

    return openInternal(report);
}


//----------------------------------------------------------------------------
// Open file.
// If repeat_count != 1, reading packets loops back to the start_offset
// until all repeat are done. If repeat_count == 0, infinite repeat.
//----------------------------------------------------------------------------

bool ts::TSFileInput::open(const UString& filename, size_t repeat_count, uint64_t start_offset, Report& report)
{
    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }

    _filename = filename;
    _repeat = repeat_count;
    _counter = 0;
    _start_offset = start_offset;
    _at_eof = false;
    _rewindable = false;

    return openInternal(report);
}


//----------------------------------------------------------------------------
// Internal open
//----------------------------------------------------------------------------

bool ts::TSFileInput::openInternal (Report& report)
{
#if defined (TS_WINDOWS)

    // Windows implementation

    if (_filename.empty()) {
        _handle = ::GetStdHandle (STD_INPUT_HANDLE);
    }
    else {
        _handle = ::CreateFile(_filename.toUTF8().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (_handle == INVALID_HANDLE_VALUE) {
            ErrorCode error_code = LastErrorCode ();
            report.log(_severity, u"cannot open file %s: %s", {_filename, ErrorCodeMessage(error_code)});
            return false;
        }
    }

    // If a repeat count or initial offset is specified, the input file must be a regular file

    if ((_repeat != 1 || _start_offset != 0) && ::GetFileType (_handle) != FILE_TYPE_DISK) {
        report.log(_severity, u"input file %s is not a regular file, cannot %s", {_filename, _repeat != 1 ? u"repeat" : u"specify start offset"});
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
            ErrorCode error_code = LastErrorCode();
            report.log(_severity, u"error seeking input file %s: %s", {_filename, ErrorCodeMessage(error_code)});
            if (!_filename.empty()) {
                ::CloseHandle(_handle);
            }
            return false;
        }
    }

#else

    // UNIX implementation

    if (_filename.empty()) {
        _fd = STDIN_FILENO;
    }
    else if ((_fd = ::open(_filename.toUTF8().c_str(), O_RDONLY | O_LARGEFILE)) < 0) {
        ErrorCode error_code = LastErrorCode();
        report.log(_severity, u"cannot open file %s: %s", {_filename, ErrorCodeMessage(error_code)});
        return false;
    }

    // If a repeat count or initial offset is specified, the input file
    // must be a regular file

    if (_repeat != 1 || _start_offset != 0) {
        struct stat st;
        if (::fstat(_fd, &st) < 0) {
            ErrorCode error_code = LastErrorCode ();
            report.log(_severity, u"cannot stat input file %s: %s", {_filename, ErrorCodeMessage(error_code)});
            if (!_filename.empty()) {
                ::close (_fd);
            }
            return false;
        }
        if (!S_ISREG(st.st_mode)) {
            report.log(_severity, u"input file %s is not a regular file, cannot %s", {_filename, _repeat != 1 ? u"repeat" : u"specify start offset"});
            if (!_filename.empty()) {
                ::close(_fd);
            }
            return false;
        }
    }

    // If an initial offset is specified, move here

    if (_start_offset != 0 && ::lseek (_fd, off_t (_start_offset), SEEK_SET) == off_t (-1)) {
        ErrorCode error_code = LastErrorCode ();
        report.log (_severity, u"error seeking input file %s: %s", {_filename, ErrorCodeMessage(error_code)});
        if (!_filename.empty()) {
            ::close (_fd);
        }
        return false;
    }

#endif

    _is_open = true;
    _total_packets = 0;
    return true;
}


//----------------------------------------------------------------------------
// Internal seek. Rewind to specified start offset plus specified index.
//----------------------------------------------------------------------------

bool ts::TSFileInput::seekInternal(uint64_t index, Report& report)
{
#if defined (TS_WINDOWS)
    // In Win32, LARGE_INTEGER is a 64-bit structure, not an integer type
    uint64_t where = _start_offset + index;
    ::LARGE_INTEGER offset(*(::LARGE_INTEGER*)(&where));
    if (::SetFilePointerEx(_handle, offset, NULL, FILE_BEGIN) == 0) {
#else
    if (::lseek(_fd, off_t(_start_offset + index), SEEK_SET) == off_t(-1)) {
#endif
        ErrorCode error_code = LastErrorCode();
        report.log(_severity, u"error seeking input file %s: %s", {_filename, ErrorCodeMessage(error_code)});
        return false;
    }
    else {
        _at_eof = false;
        return true;
    }
}


//----------------------------------------------------------------------------
// Seek the file to the specified packet_index (plus the previously specified start_offset).
// The file must have been open in rewindable mode.
//----------------------------------------------------------------------------

bool ts::TSFileInput::seek(PacketCounter packet_index, Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }
    else if (!_rewindable) {
        report.log(_severity, u"input file %s is not rewindable", {_filename});
        return false;
    }
    else {
        return seekInternal(packet_index * PKT_SIZE, report);
    }
}


//----------------------------------------------------------------------------
// Close file.
//----------------------------------------------------------------------------

bool ts::TSFileInput::close(Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }

    if (!_filename.empty()) {
#if defined (TS_WINDOWS)
        ::CloseHandle(_handle);
#else
        ::close(_fd);
#endif
    }

    _is_open = false;
    _total_packets = 0;
    _filename.clear();

    return true;
}


//----------------------------------------------------------------------------
// Read TS packets. Return the actual number of read packets.
// Returning zero means error or end of file repetition.
//----------------------------------------------------------------------------

size_t ts::TSFileInput::read(TSPacket* buffer, size_t max_packets, Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return 0;
    }

    if (_at_eof) {
        return 0;
    }

    char* data = reinterpret_cast <char*> (buffer);
    const size_t req_size = max_packets * PKT_SIZE;
    size_t got_size = 0;
    bool got_error = false;
    ErrorCode error_code = 0;

    // Loop on read until we get enough
    while (got_size < req_size && !_at_eof && !got_error) {

#if defined (TS_WINDOWS)
        // Windows implementation
        ::DWORD insize;
        if (::ReadFile (_handle, data + got_size, ::DWORD (req_size - got_size), &insize, NULL)) {
            // Normal case: some data were read
            got_size += insize;
            assert (got_size <= req_size);
            _at_eof = insize == 0;
        }
        else {
            error_code = LastErrorCode ();
            _at_eof = error_code == ERROR_HANDLE_EOF || error_code == ERROR_BROKEN_PIPE;
            got_error = !_at_eof;
        }
#else
        // UNIX implementation
        ssize_t insize = ::read (_fd, data + got_size, req_size - got_size);
        if (insize > 0) {
            // Normal case: some data were read
            got_size += insize;
            assert (got_size <= req_size);
        }
        else if (insize == 0) {
            _at_eof = true;
        }
        else if ((error_code = LastErrorCode ()) != EINTR) {
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
    _total_packets += count;
    return count;
}

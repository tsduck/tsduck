//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Transport stream file output
//
//----------------------------------------------------------------------------

#include "tsTSFileOutput.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TSFileOutput::TSFileOutput() :
    _filename(),
    _is_open(false),
    _severity(Severity::Error),
    _total_packets(0),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE)
#else
    _fd(-1)
#endif
{
}


//----------------------------------------------------------------------------
// Open method
//----------------------------------------------------------------------------

bool ts::TSFileOutput::open(const UString& filename, bool append, bool keep, Report& report)
{
    if (_is_open) {
        report.log(_severity, u"already open");
        return false;
    }

    _filename = filename;
    bool got_error = false;
    ErrorCode error_code = SYS_SUCCESS;

#if defined (TS_WINDOWS)

    // Windows implementation
    ::DWORD flags;
    if (keep) {
        flags = CREATE_NEW;
    }
    else if (append) {
        flags = OPEN_ALWAYS;
    }
    else {
        flags = CREATE_ALWAYS;
    }

    if (_filename.empty()) {
        _handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    }
    else {
        // Create file
        _handle = ::CreateFile(_filename.toUTF8().c_str(), GENERIC_WRITE, 0, NULL, flags, FILE_ATTRIBUTE_NORMAL, NULL);
        got_error = _handle == INVALID_HANDLE_VALUE;
        error_code = LastErrorCode();
        // Move to end of file if --append
        if (!got_error && append && ::SetFilePointer(_handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
            // SetFilePointer error
            got_error = true;
            error_code = LastErrorCode();
            ::CloseHandle(_handle);
        }
    }

#else

    // UNIX implementation
    int flags = O_CREAT | O_WRONLY | O_LARGEFILE;
    const mode_t mode = 0666; // -rw-rw-rw (minus umask)

    if (keep) {
        flags |= O_EXCL;
    }
    else if (append) {
        flags |= O_APPEND;
    }
    else {
        flags |= O_TRUNC;
    }

    if (_filename.empty()) {
        _fd = STDOUT_FILENO;
    }
    else {
        _fd = ::open(_filename.toUTF8().c_str(), flags, mode);
        got_error = _fd < 0;
        error_code = LastErrorCode();
        report.debug(u"creating file %s, fd=%d, error_code=%d", {filename, _fd, error_code});
    }

#endif

    if (got_error) {
        report.log(_severity, u"cannot create output file %s: %s", {_filename, ErrorCodeMessage(error_code)});
    }

    _total_packets = 0;
    return _is_open = !got_error;
}


//----------------------------------------------------------------------------
// Close method
//----------------------------------------------------------------------------

bool ts::TSFileOutput::close(Report& report)
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
    return true;
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TSFileOutput::~TSFileOutput()
{
    if (_is_open) {
        close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Write method
//----------------------------------------------------------------------------

bool ts::TSFileOutput::write(const TSPacket* buffer, size_t packet_count, Report& report)
{
    if (!_is_open) {
        report.log(_severity, u"not open");
        return false;
    }

    // Loop on write until everything is gone

    bool got_error = false;
    ErrorCode error_code = SYS_SUCCESS;
    const char* const data_buffer = reinterpret_cast <const char*> (buffer);
    const char* data = data_buffer;

#if defined (TS_WINDOWS)

    // Windows implementation

    ::DWORD remain = ::DWORD (packet_count) * PKT_SIZE;
    ::DWORD outsize;

    while (remain > 0 && !got_error) {
        if (::WriteFile (_handle, data, remain, &outsize, NULL) != 0)  {
            // Normal case, some data were written
            outsize = std::min (outsize, remain);
            data += outsize;
            remain -= std::max (remain, outsize);
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
    ssize_t outsize;

    while (remain > 0 && !got_error) {
        outsize = ::write (_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            assert (size_t (outsize) <= remain);
            data += outsize;
            remain -= std::max (remain, size_t (outsize));
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            report.debug(u"write error on %s, fd=%d, error_code=%d", {_filename, _fd, error_code});
            got_error = true;
            if (error_code == EPIPE) {
                // Broken pipe: keep the error state but don't report error.
                error_code = SYS_SUCCESS;
            }
        }
    }

#endif

    if (got_error && error_code != SYS_SUCCESS) {
        report.log(_severity, u"error writing output file %s: %s (%d)", {_filename, ErrorCodeMessage(error_code), error_code});
    }

    _total_packets += (data - data_buffer) / PKT_SIZE;
    return !got_error;
}

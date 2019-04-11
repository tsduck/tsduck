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

#include "tsDuckContext.h"
#include "tsCerrReport.h"
#include "tsDVBCharset.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructors.
//----------------------------------------------------------------------------

ts::DuckContext::DuckContext(std::ostream* output, Report* report) :
    _report(report != nullptr ? report : CerrReport::Instance()),
    _out(output != nullptr ? output : &std::cout),
    _outFile(),
    _dvbCharsetIn(nullptr),
    _dvbCharsetOut(nullptr)
{
}


//----------------------------------------------------------------------------
// Flush the text output.
//----------------------------------------------------------------------------

void ts::DuckContext::flush()
{
    // Flush the output.
    _out->flush();

    // On Unix, we must force the lower-level standard output.
#if defined(TS_UNIX)
    if (_out == &std::cout) {
        ::fflush(stdout);
        ::fsync(STDOUT_FILENO);
    }
    else if (_out == &std::cerr) {
        ::fflush(stderr);
        ::fsync(STDERR_FILENO);
    }
#endif
}


//----------------------------------------------------------------------------
// Redirect the output stream to a file.
//----------------------------------------------------------------------------

void ts::DuckContext::redirect(std::ostream* stream, bool override)
{
    // Do not override output is not standard output (or explicit override).
    if (override || _out == &std::cout) {
        if (_out == &_outFile) {
            _outFile.close();
        }
        _out = stream == nullptr ? &std::cout : stream;
    }
}

bool ts::DuckContext::redirect(const UString& fileName, bool override)
{
    // Do not override output is not standard output (or explicit override).
    if (override || _out == &std::cout) {
        // Close previous file, if any.
        if (_out == &_outFile) {
            _outFile.close();
            _out = &std::cout;
        }

        // Open new file if any.
        if (!fileName.empty()) {
            _report->verbose(u"creating %s", {fileName});
            const std::string nameUTF8(fileName.toUTF8());
            _outFile.open(nameUTF8.c_str(), std::ios::out);
            if (!_outFile) {
                _report->error(u"cannot create %s", {fileName});
                return false;
            }
            _out = &_outFile;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// A method to display data if it can be interpreted as an ASCII string.
//----------------------------------------------------------------------------

std::ostream& ts::DuckContext::displayIfASCII(const void *data, size_t size, const UString& prefix, const UString& suffix)
{
    const std::string ascii(toASCII(data, size));
    if (!ascii.empty()) {
        (*_out) << prefix << ascii << suffix;
    }
    return *_out;
}


//----------------------------------------------------------------------------
// A utility method to interpret data as an ASCII string.
//----------------------------------------------------------------------------

std::string ts::DuckContext::toASCII(const void *data, size_t size) const
{
    const char* str = reinterpret_cast<const char*>(data);
    size_t strSize = 0;

    for (size_t i = 0; i < size; ++i) {
        if (str[i] >= 0x20 && str[i] <= 0x7E) {
            // This is an ASCII character.
            if (i == strSize) {
                strSize++;
            }
            else {
                // But come after trailing zero.
                return std::string();
            }
        }
        else if (str[i] != 0) {
            // Not ASCII, not trailing zero, unusable string.
            return std::string();
        }
    }

    // Found an ASCII string.
    return std::string(str, strSize);
}

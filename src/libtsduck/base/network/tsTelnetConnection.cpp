//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard, Frederic Peignot
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

#include "tsTelnetConnection.h"

// A telnet end-of-line sequence.
const std::string ts::TelnetConnection::EOL("\r\n");


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TelnetConnection::TelnetConnection(const std::string& prompt) :
    TCPConnection(),
    _buffer(),
    _prompt(prompt)
{
    // Maximum size we may read per line.
    _buffer.reserve(4096);
}

ts::TelnetConnection::~TelnetConnection()
{
}


//----------------------------------------------------------------------------
// Send a request to the server.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::send(const std::string& str, Report& report)
{
    return SuperClass::send(str.c_str(), str.size(), report);
}

bool ts::TelnetConnection::send(const UString& str, Report& report)
{
    return send(str.toUTF8(), report);
}

bool ts::TelnetConnection::sendLine(const std::string& str, Report& report)
{
    return send(str, report) && send(EOL, report);
}

bool ts::TelnetConnection::sendLine(const UString& str, Report& report)
{
    return send(str, report) && send(EOL, report);
}


//----------------------------------------------------------------------------
// Implementation of Report.
//----------------------------------------------------------------------------

void ts::TelnetConnection::writeLog(int severity, const UString& msg)
{
    sendLine(Severity::Header(severity) + msg, NULLREP);
}


//----------------------------------------------------------------------------
// Receive all characters until a delimitor has been received.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::waitForChunk(const std::string& eol, std::string& data, const AbortInterface* abort, Report& report)
{
    // Already allocated memory.
    const size_t capacity = _buffer.capacity();

    // While a full line has not been received yet
    for (;;) {
        // Check first that what we are looking for is not yet in the buffer
        // If no EOL is specified, return what is in the buffer if not empty.
        const size_t eol_index = eol.empty() ? _buffer.size() : _buffer.find(eol);
        if (eol_index != NPOS && (!eol.empty() || eol_index > 0)) {
            assert(eol_index + eol.size() <= _buffer.size());
            data = _buffer.substr(0, eol_index);
            _buffer.erase(0, eol_index + eol.size());
            return true;
        }

        // Do not read more than the planned capacity of the buffer.
        // If the whole capacity is filled without EOL, return the buffer.
        const size_t previous_size = _buffer.size();
        if (previous_size >= capacity) {
            data = _buffer;
            _buffer.clear();
            return true;
        }

        // EOL not yet received, read some data from the socket.
        _buffer.resize(capacity);
        size_t size = 0;
        const bool result = SuperClass::receive(&_buffer[previous_size], capacity - previous_size, size, abort, report);
        _buffer.resize(previous_size + size);

        // In case of error, either return what is in the buffer or an error.
        if (!result || size == 0) {
            data = _buffer;
            return !data.empty();
        }
    }
}


//----------------------------------------------------------------------------
// Receive a prompt.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::waitForPrompt(const AbortInterface* abort, Report& report)
{
    std::string unused;
    return _prompt.empty() || waitForChunk(_prompt, unused, abort, report);
}


//----------------------------------------------------------------------------
// Receive a line.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::receive(std::string& data, const AbortInterface* abort, Report& report)
{
    return waitForChunk(std::string(), data, abort, report);
}

bool ts::TelnetConnection::receive(UString& data, const AbortInterface* abort, Report& report)
{
    std::string sdata;
    const bool result = receive(sdata, abort, report);
    if (result) {
        data.assignFromUTF8(sdata);
    }
    else {
        data.clear();
    }
    return result;
}

bool ts::TelnetConnection::receiveLine(std::string& line, const AbortInterface* abort, Report& report)
{
    // Read until new-line (end of EOL).
    if (!waitForChunk("\n", line, abort, report)) {
        return false;
    }

    // Cleanup trailing CR LF.
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    return true;
}

bool ts::TelnetConnection::receiveLine(UString& line, const AbortInterface* abort, Report& report)
{
    std::string sline;
    const bool result = receiveLine(sline, abort, report);
    if (result) {
        line.assignFromUTF8(sline);
    }
    else {
        line.clear();
    }
    return result;
}

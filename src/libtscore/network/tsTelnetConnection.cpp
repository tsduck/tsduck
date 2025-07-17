//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard, Frederic Peignot
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTelnetConnection.h"
#include "tsNullReport.h"

// A telnet end-of-line sequence.
const std::string ts::TelnetConnection::EOL("\r\n");


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TelnetConnection::TelnetConnection(TCPConnection& connection, const std::string& prompt) :
    _connection(connection),
    _prompt(prompt)
{
    // Maximum size we may read per line.
    _buffer.reserve(4096);
}

ts::TelnetConnection::~TelnetConnection()
{
}

bool ts::TelnetConnection::reset()
{
    _buffer.clear();
    return true;
}


//----------------------------------------------------------------------------
// Send a request to the server.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::sendText(const std::string& str, Report& report)
{
    return _connection.send(str.c_str(), str.size(), report);
}

bool ts::TelnetConnection::sendText(const UString& str, Report& report)
{
    return sendText(str.toUTF8(), report);
}

bool ts::TelnetConnection::sendLine(const std::string& str, Report& report)
{
    return sendText(str, report) && sendText(EOL, report);
}

bool ts::TelnetConnection::sendLine(const UString& str, Report& report)
{
    return sendText(str, report) && sendText(EOL, report);
}


//----------------------------------------------------------------------------
// Implementation of Report.
//----------------------------------------------------------------------------

void ts::TelnetConnection::writeLog(int severity, const UString& msg)
{
    sendLine(Severity::Header(severity) + msg, NULLREP);
}


//----------------------------------------------------------------------------
// Get currently buffered input data and flush that buffer.
//----------------------------------------------------------------------------

void ts::TelnetConnection::getAndFlush(ByteBlock& data)
{
    data.copy(_buffer.data(), _buffer.size());
    _buffer.clear();
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
        const bool result = _connection.receive(&_buffer[previous_size], capacity - previous_size, size, abort, report);
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

bool ts::TelnetConnection::receiveText(std::string& data, const AbortInterface* abort, Report& report)
{
    return waitForChunk(std::string(), data, abort, report);
}

bool ts::TelnetConnection::receiveText(UString& data, const AbortInterface* abort, Report& report)
{
    std::string sdata;
    const bool result = receiveText(sdata, abort, report);
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

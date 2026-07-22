//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard, Frederic Peignot
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTextStream.h"
#include "tsByteBlock.h"
#include "tsSocket.h"

// The Telnet protocol defines CR-LF as end-of-line sequence.
const std::string ts::TextStream::DEFAULT_EOL("\r\n");


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TextStream::TextStream(StreamInterface& stream, const std::string& eol, const std::string& prompt) :
    _stream(stream),
    _eol(eol),
    _prompt(prompt)
{
    // Maximum size we may read per line.
    _buffer.reserve(4096);

    // Get notified of connection and disconnection if the device is a socket.
    Socket* sock = dynamic_cast<Socket*>(&stream);
    if (sock != nullptr) {
        sock->addSubscription(this);
    }
}


//----------------------------------------------------------------------------
// Clear the flushable buffer on connect and disconnect.
//----------------------------------------------------------------------------

void ts::TextStream::handleSocketConnected(TCPConnection& sock)
{
    reset();
}

void ts::TextStream::handleSocketDisconnected(TCPConnection& sock, bool silent)
{
    reset();
}


//----------------------------------------------------------------------------
// Send a request to the server.
//----------------------------------------------------------------------------

bool ts::TextStream::writeText(const std::string& str)
{
    return _stream.writeStream(str.c_str(), str.size());
}

bool ts::TextStream::writeText(const UString& str)
{
    return writeText(str.toUTF8());
}

bool ts::TextStream::writeLine(const std::string& str)
{
    return writeText(str) && writeText(_eol);
}

bool ts::TextStream::writeLine(const UString& str)
{
    return writeText(str) && writeText(_eol);
}


//----------------------------------------------------------------------------
// Implementation of Report.
//----------------------------------------------------------------------------

void ts::TextStream::writeLog(int severity, const UString& msg)
{
    writeLine(Severity::AddHeader(severity, msg));
}


//----------------------------------------------------------------------------
// Get currently buffered input data and flush that buffer.
//----------------------------------------------------------------------------

void ts::TextStream::getAndFlush(ByteBlock& data)
{
    data.copy(_buffer.data(), _buffer.size());
    _buffer.clear();
}


//----------------------------------------------------------------------------
// Receive all characters until a delimitor has been received.
//----------------------------------------------------------------------------

bool ts::TextStream::waitForChunk(const std::string& eol, std::string& data, const AbortInterface* abort)
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

        // EOL not yet received, read some data from the stream.
        _buffer.resize(capacity);
        size_t size = 0;
        const bool result = _stream.readStream(&_buffer[previous_size], capacity - previous_size, size, abort);
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

bool ts::TextStream::waitForPrompt(const AbortInterface* abort)
{
    std::string unused;
    return _prompt.empty() || waitForChunk(_prompt, unused, abort);
}


//----------------------------------------------------------------------------
// Receive a line.
//----------------------------------------------------------------------------

bool ts::TextStream::readText(std::string& data, const AbortInterface* abort)
{
    return waitForChunk(std::string(), data, abort);
}

bool ts::TextStream::readText(UString& data, const AbortInterface* abort)
{
    std::string sdata;
    const bool result = readText(sdata, abort);
    if (result) {
        data.assignFromUTF8(sdata);
    }
    else {
        data.clear();
    }
    return result;
}

bool ts::TextStream::readLine(std::string& line, const AbortInterface* abort)
{
    // Read until new-line (end of EOL).
    if (!waitForChunk("\n", line, abort)) {
        return false;
    }

    // Cleanup trailing CR LF.
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    return true;
}

bool ts::TextStream::readLine(UString& line, const AbortInterface* abort)
{
    std::string sline;
    const bool result = readLine(sline, abort);
    if (result) {
        line.assignFromUTF8(sline);
    }
    else {
        line.clear();
    }
    return result;
}

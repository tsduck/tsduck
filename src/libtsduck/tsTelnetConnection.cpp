//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard, Frederic Peignot
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
#include "tsHexa.h"


//----------------------------------------------------------------------------
//  Configuration & constants
//----------------------------------------------------------------------------

const std::string EOL = "\n";


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TelnetConnection::TelnetConnection(const std::string prompt) :
    TCPConnection(),
    _received(0),
    _prompt(prompt),
    _mutex()
{
}


bool ts::TelnetConnection::send(const std::string& str, ReportInterface& report)
{
    return SuperClass::send(str.c_str(), str.size(), report);
}

//----------------------------------------------------------------------------
// receive all characters until a delimitor has been received and returns
// everything up to the delimitor.
//----------------------------------------------------------------------------

bool ts::TelnetConnection::waitForChunk(const std::string eol, std::string& found, const AbortInterface* abort, ReportInterface& report)
{
    size_t size = 0;
    size_t eol_size = eol.length();
    // while a full line has not been received yet
    while (1) {
        // Checks first that what we are looking for is not yet in the buffer
        if (_received > 0) {
            // create a string with the buffer
            std::string str_buffer = std::string(_buffer, _received);

            // for all substring of the buffer
            for (size_t i = 0; i <= str_buffer.length() - eol_size; i++) {

                // extract a substring of the size of the matching pattern
                std::string sub_str = str_buffer.substr(i, eol_size);

                if (sub_str.compare(eol) == 0) {
                    found = str_buffer.substr(0, i);
                    size_t prompt_length = i + eol_size; // size of everything from start to marker (included)
                    ::memcpy(_buffer, &_buffer[prompt_length], _received - prompt_length);
                    _received -= prompt_length;
                    return true;
                }
            }
        }

        // what we are looking for does not exist yet
        // read some data from the socket
        bool result = SuperClass::receive((void *) &_buffer[_received], BUFFER_SIZE - _received, size, abort, report);

        if (!result || !size) {
            report.info("result = false\n");
            return result;
        }

        _received += size;
    }
}

bool ts::TelnetConnection::waitForPrompt(const AbortInterface* abort, ReportInterface& report)
{
    std::string found = "";
    bool result = waitForChunk(_prompt, found, abort, report);
    return result;
}

bool ts::TelnetConnection::receive(std::string& found, const AbortInterface* abort, ReportInterface& report)
{
    bool result = waitForChunk(_prompt, found, abort, report);
    return result;
}

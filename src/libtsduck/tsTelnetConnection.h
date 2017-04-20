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
//!
//!  @file
//!  Implementation of a half-duplex line oriented telnet connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"
#include "tsMutex.h"

namespace ts {

    // This class supports the communication with a half-duplex line oriented telnet server.
    // The server sends a prompt.
    // The client sends a request.
    // The server replies by one or more lines followed by the prompt.

    // From the client point of view the interface must allow
    // To send a request
    // To get replies line until all the lines of the replies have been read.

    class TSDUCKDLL TelnetConnection: public TCPConnection
    {
    public:
        typedef TCPConnection SuperClass;

        // Constructor.
        TelnetConnection(const std::string prompt);

        // Send a request to the server
        bool send(const std::string&, ReportInterface&);

        // Receive a line
        // return true until the last line of the replies has been received.
        bool receive(std::string&, const AbortInterface*, ReportInterface&);

        // Receive a prompt
        bool waitForPrompt(const AbortInterface*, ReportInterface&);

    private:
        TelnetConnection(const TelnetConnection&) = delete;
        TelnetConnection& operator=(const TelnetConnection&) = delete;

        static const size_t BUFFER_SIZE = 1024 * 4;
        char        _buffer[BUFFER_SIZE];
        size_t      _received;
        std::string _prompt;
        Mutex       _mutex;

        bool waitForChunk(const std::string, std::string&, const AbortInterface*, ReportInterface&);
    };
}

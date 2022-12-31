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
//!
//!  @file
//!  Implementation of a half-duplex line oriented telnet connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"
#include "tsUString.h"
#include "tsMutex.h"

namespace ts {
    //!
    //! Implementation of a half-duplex line oriented telnet connection.
    //! @ingroup net
    //!
    //! This class supports the communication with a half-duplex line oriented telnet server:
    //! - The server sends a prompt.
    //! - The client sends a request.
    //! - The server replies by one or more lines followed by the prompt.
    //!
    //! From the client point of view the interface must allow:
    //! - To send a request.
    //! - To get replies line until all the lines of the replies have been read.
    //!
    //! This class is also a subclass of Report, allowing it to be used to end
    //! log messages.
    //!
    class TSDUCKDLL TelnetConnection: public TCPConnection, public Report
    {
        TS_NOCOPY(TelnetConnection);
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef TCPConnection SuperClass;

        //!
        //! Constructor.
        //! @param [in] prompt Prompt string to send to the client.
        //!
        TelnetConnection(const std::string& prompt = std::string());

        //!
        //! Virtual destructor
        //!
        virtual ~TelnetConnection() override;

        //!
        //! Send a string to the server.
        //! @param [in] str The string to send to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool send(const std::string& str, Report& report);

        //!
        //! Send a string to the server.
        //! @param [in] str The string to send to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool send(const UString& str, Report& report);

        //!
        //! Send a text line to the server.
        //! @param [in] str The line to send to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool sendLine(const std::string& str, Report& report);

        //!
        //! Send a text line to the server.
        //! @param [in] str The line to send to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool sendLine(const UString& str, Report& report);

        //!
        //! Receive character data.
        //! @param [out] data The received data.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool receive(std::string& data, const AbortInterface* abort, Report& report);

        //!
        //! Receive character data.
        //! @param [out] data The received data.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool receive(UString& data, const AbortInterface* abort, Report& report);

        //!
        //! Receive a line.
        //! @param [out] line The received line.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool receiveLine(std::string& line, const AbortInterface* abort, Report& report);

        //!
        //! Receive a line.
        //! @param [out] line The received line.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool receiveLine(UString& line, const AbortInterface* abort, Report& report);

        //!
        //! Receive a prompt.
        //! Do not wait if the prompt is empty.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool waitForPrompt(const AbortInterface* abort, Report& report);

        //!
        //! A telnet end-of-line sequence.
        //!
        static const std::string EOL;

    protected:
        // Implementation of Report.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        std::string _buffer;
        std::string _prompt;

        // Receive all characters until a delimitor has been received.
        bool waitForChunk(const std::string& eol, std::string& data, const AbortInterface*, Report&);
    };
}

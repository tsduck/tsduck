//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard, Frederic Peignot
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

namespace ts {
    //!
    //! Implementation of a half-duplex line oriented telnet connection.
    //! @ingroup libtscore net
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
    class TSCOREDLL TelnetConnection: public Report
    {
        TS_NOBUILD_NOCOPY(TelnetConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] connection The underlying connection.
        //! A reference is kept in this instance.
        //! @param [in] prompt Prompt string to send to the client.
        //!
        TelnetConnection(TCPConnection& connection, const std::string& prompt = std::string());

        //!
        //! Virtual destructor
        //!
        virtual ~TelnetConnection() override;

        //!
        //! Get a reference to the associated TCPConnection.
        //! @return A reference to the associated TCPConnection.
        //!
        TCPConnection& connection() { return _connection; }

        //!
        //! Reset the internal buffer.
        //! If the underlying TCPConnection is reused for several connections,
        //! reset() should be called each time a new connection is established.
        //! Otherwise, the new connection would reuse unread bytes from the
        //! previous connection.
        //! @return Always true.
        //!
        bool reset();

        //!
        //! Send a string to the server.
        //! @param [in] str The string to sendText to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool sendText(const std::string& str, Report& report);

        //!
        //! Send a string to the server.
        //! @param [in] str The string to sendText to the server.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool sendText(const UString& str, Report& report);

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
        bool receiveText(std::string& data, const AbortInterface* abort, Report& report);

        //!
        //! Receive character data.
        //! @param [out] data The received data.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool receiveText(UString& data, const AbortInterface* abort, Report& report);

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
        //! Get currently buffered input data and flush that buffer.
        //! This method is useful when TCP connection switches from text mode (telnet protocol)
        //! to binary mode. The returned data are the start of the input binary data. The remaining
        //! data can be received using methods from the parent class TCPConnection.
        //! @param [out] data Currently buffered data.
        //!
        void getAndFlush(ByteBlock& data);

        //!
        //! A telnet end-of-line sequence.
        //!
        static const std::string EOL;

    protected:
        // Implementation of Report.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        TCPConnection& _connection;
        std::string    _buffer {};
        std::string    _prompt {};

        // Receive all characters until a delimitor has been received.
        bool waitForChunk(const std::string& eol, std::string& data, const AbortInterface*, Report&);
    };
}

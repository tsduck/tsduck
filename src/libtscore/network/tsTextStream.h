//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard, Frederic Peignot
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a half-duplex line-oriented Telnet-like connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocketHandlerInterface.h"
#include "tsStreamInterface.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Implementation of a half-duplex line-oriented Telnet-like connection.
    //! @ingroup libtscore net
    //!
    //! This class supports the communication with a half-duplex line-oriented Telnet-like server:
    //! - The server sends a prompt.
    //! - The client sends a request.
    //! - The server replies by one or more lines followed by the prompt.
    //!
    //! From the client point of view the interface must allow:
    //! - To send a request.
    //! - To get replies line until all the lines of the replies have been read.
    //!
    //! This class is also a subclass of Report, allowing it to be used to send log messages.
    //!
    //! Although this class is typically used on a TCPConnection (or a subclass of it), it can be
    //! used on any class implementing StreamInterface. It the underlying device is not interactive
    //! (a file for instance), don't try to use a prompt of course.
    //!
    class TSCOREDLL TextStream: public Report, private SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TextStream);
    public:
        //!
        //! Constructor.
        //! @param [in,out] stream Associated stream device. The device object must remain valid as long as this object is valid.
        //! @param [in] eol End-of-file sequence to send at end of each line. Input is auto-adaptive: a line is always read up
        //! to a LF character and all previous CR characters are discarded.
        //! @param [in] prompt Prompt string to send to the client.
        //!
        explicit TextStream(StreamInterface& stream, const std::string& eol = DEFAULT_EOL, const std::string& prompt = std::string());

        //!
        //! Default end-of-line sequence (CR-LF).
        //! The Telnet protocol defines CR-LF as end-of-line sequence.
        //!
        static const std::string DEFAULT_EOL;

        //!
        //! Set a new end-of-line sequence for output lines.
        //! Input is auto-adaptive: a line is always read up to a LF character and all previous CR characters are discarded.
        //! @param [in] eol End-of-file sequence to send at end of each line.
        //!
        void setEOL(const std::string& eol) { _eol = eol; }

        //!
        //! Get a reference to the associated stream device.
        //! @return A reference to the associated stream device.
        //!
        StreamInterface& stream() { return _stream; }

        //!
        //! Reset the internal buffer.
        //!
        //! If the underlying stream device is reused for several connections or session, reset() should be called each time a
        //! new connection is established. Otherwise, the new connection would reuse unread bytes from the previous connection.
        //!
        //! Note that if the stream device is a TCPConnection or a subclass of it, the reset is automatically performed when
        //! the socket connects or disconnects.
        //!
        void reset() { _buffer.clear(); }

        //!
        //! Send a string to the server.
        //! @param [in] str The string to writeText to the server.
        //! @return True on success, false on error.
        //!
        bool writeText(const std::string& str);

        //!
        //! Send a string to the server.
        //! @param [in] str The string to writeText to the server.
        //! @return True on success, false on error.
        //!
        bool writeText(const UString& str);

        //!
        //! Send a text line to the server.
        //! @param [in] str The line to send to the server.
        //! @return True on success, false on error.
        //!
        bool writeLine(const std::string& str);

        //!
        //! Send a text line to the server.
        //! @param [in] str The line to send to the server.
        //! @return True on success, false on error.
        //!
        bool writeLine(const UString& str);

        //!
        //! Receive character data.
        //! @param [out] data The received data.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool readText(std::string& data, const AbortInterface* abort = nullptr);

        //!
        //! Receive character data.
        //! @param [out] data The received data.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool readText(UString& data, const AbortInterface* abort = nullptr);

        //!
        //! Receive a line.
        //! @param [out] line The received line.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool readLine(std::string& line, const AbortInterface* abort = nullptr);

        //!
        //! Receive a line.
        //! @param [out] line The received line.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //! Return true until the last line of the replies has been received.
        //!
        bool readLine(UString& line, const AbortInterface* abort = nullptr);

        //!
        //! Receive a prompt.
        //! Do not wait if the prompt is empty.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        bool waitForPrompt(const AbortInterface* abort = nullptr);

        //!
        //! Get currently buffered input data and flush that buffer.
        //! This method is useful when TCP connection switches from text mode (Telnet protocol)
        //! to binary mode. The returned data are the start of the input binary data. The remaining
        //! data can be received using methods from the parent class TCPConnection.
        //! @param [out] data Currently buffered data.
        //!
        void getAndFlush(ByteBlock& data);

    protected:
        // Implementation of Report.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        StreamInterface& _stream;
        std::string      _buffer {};
        std::string      _eol {DEFAULT_EOL};
        std::string      _prompt {};

        // Receive all characters until a delimitor has been received.
        bool waitForChunk(const std::string& eol, std::string& data, const AbortInterface*);

        // Implementation of socket interface.
        virtual void handleSocketConnected(TCPConnection& sock) override;
        virtual void handleSocketDisconnected(TCPConnection& sock, bool silent) override;
    };
}

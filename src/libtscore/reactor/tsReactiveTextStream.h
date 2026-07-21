//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Line-oriented Telnet-like connection for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTextStreamHandlerInterface.h"
#include "tsSocketHandlerInterface.h"
#include "tsReactiveStream.h"
#include "tsTextStream.h"

namespace ts {
    //!
    //! Line-oriented Telnet-like connection for use in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTextStream is a wrapper around ReactiveStream to handle reactive I/O.
    //!
    class TSCOREDLL ReactiveTextStream: private ReactiveStreamHandlerInterface, private SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTextStream);
    public:
        //!
        //! Constructor.
        //! @param [in,out] stream Associated stream device. The device object must remain valid as long as this object is valid.
        //! @param [in] eol End-of-file sequence to send at end of each line. Input is auto-adaptive: a line is always read up
        //! to a LF character and all previous CR characters are discarded.
        //!
        explicit ReactiveTextStream(ReactiveStream& stream, const std::string& eol = TextStream::DEFAULT_EOL);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveTextStream() override;

        //!
        //! Set a new end-of-line sequence for output lines.
        //! Input is auto-adaptive: a line is always read up to a LF character and all previous CR characters are discarded.
        //! @param [in] eol End-of-file sequence to send at end of each line.
        //!
        void setEOL(const std::string& eol) { _eol = eol; }

        //!
        //! Get a reference to the associated reactive stream device.
        //! @return A reference to the associated reactive stream device.
        //!
        ReactiveStream& stream() { return _stream; }

        //!
        //! Start the operation of sending a line of text over the stream device.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] line Text line to send. The corresponding memory area is no longer used upon return.
        //! An end-of-line marker is automatically added.
        //! @param [in] flush If false, data are buffered into the ReactiveTextStream and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startWriteLine(const std::string& line, bool flush = true);

        //!
        //! Start the operation of sending a line of text over the stream device.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] line Text line to send. The corresponding memory area is no longer used upon return.
        //! An end-of-line marker is automatically added.
        //! @param [in] flush If false, data are buffered into the ReactiveTextStream and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startWriteLine(const UString& line, bool flush = true);

        //!
        //! Start the operation of sending text over the stream device.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] text Text to send. The corresponding memory area is no longer used upon return. No end-of-line marker is added.
        //! @param [in] flush If false, data are buffered into the ReactiveTextStream and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startWriteText(const std::string& text, bool flush = true);

        //!
        //! Start the operation of sending text over the stream device.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] text Text to send. The corresponding memory area is no longer used upon return. No end-of-line marker is added.
        //! @param [in] flush If false, data are buffered into the ReactiveTextStream and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startWriteText(const UString& text, bool flush = true);

        //!
        //! Start the operation of receiving messages from the socket.
        //! @param [in] handler Handler class to call each time a text line is received. The method handleTextLine() will
        //! be called on each received line. Cannot be null. If a previous receive handler was registered, it is replaced.
        //! @param [in] buffer_size Size of input buffers to receive data.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReadText(ReactiveTextStreamHandlerInterface* handler, size_t buffer_size = ReactiveStream::DEFAULT_RECEIVE_BUFFER_SIZE);

    private:
        // The send user-data is a buffer containing the formatted binary data to send.
        // As long as we don't flush, we append in the _unflushed_data buffer.
        // When we flush, the buffer must not change until the I/O is in progress.
        class TSCOREDLL SendUserData: public Object
        {
        public:
            std::string buffer{};
            virtual ~SendUserData() override;
        };
        using SendUserDataPtr = std::shared_ptr<SendUserData>;

        // ReactiveTextStream private fields.
        ReactiveStream&                     _stream;
        ReactiveTextStreamHandlerInterface* _receive_handler = nullptr;
        SendUserDataPtr                     _unflushed_data {};
        std::string                         _eol {TextStream::DEFAULT_EOL};

        // Get and send a formatted buffer.
        SendUserDataPtr getBuffer(bool flush);
        bool startSendData(SendUserDataPtr& buf, bool eol, bool flush);

        // Inherited methods.
        virtual void handleReadStream(ReactiveStream& stream, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data) override;
        virtual void handleSocketConnected(TCPConnection& sock) override;
        virtual void handleSocketDisconnected(TCPConnection& sock, bool silent) override;
    };
}

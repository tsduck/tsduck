//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2021, Anthony Delannoy
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
//!  Secure Reliable Transport (SRT) Socket.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocketAddress.h"
#include "tsUString.h"
#include "tsIPUtils.h"
#include "tsReport.h"
#include "tsArgsSupplierInterface.h"

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) socket mode
    //!
    enum class SRTSocketMode : int {
        DEFAULT    = -1,  //!< Unspecified, use command line mode.
        LISTENER   =  0,  //!< Listener mode.
        CALLER     =  1,  //!< Caller mode.
        RENDEZVOUS =  2,  //!< Rendez-vous mode (unsupported).
        LEN        =  3,  //!< Unknown.
    };

    //!
    //! Secure Reliable Transport (SRT) Socket.
    //! If the libsrt is not available during compilation of this class,
    //! all methods will fail with an error status.
    //! @see https://github.com/Haivision/srt
    //! @see https://www.srtalliance.org/
    //! @ingroup net
    //!
    class TSDUCKDLL SRTSocket: public ArgsSupplierInterface
    {
        TS_NOCOPY(SRTSocket);
    public:
        //!
        //! Constructor.
        //!
        SRTSocket();

        //!
        //! Destructor.
        //!
        ~SRTSocket() override;

        //!
        //! Open the socket using parameters from the command line.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(Report& report = CERR)
        {
            return open(SRTSocketMode::DEFAULT, SocketAddress(), SocketAddress(), report);
        }

        //!
        //! Open the socket.
        //! @param [in] mode SRT socket mode. If set to DEFAULT, the mode must have been specified in the command line options.
        //! @param [in] local_address Local socket address. Ignored in DEFAULT and CALLER modes.
        //! @param [in] remote_address Remote socket address. Ignored in DEFAULT and LISTENER modes.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(SRTSocketMode mode, const SocketAddress& local_address, const SocketAddress& remote_address, Report& report = CERR);

        //!
        //! Close the socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool close(Report& report = CERR);

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! Preset local and remote socket addresses in string form.
        //! - If both strings are empty, the current mode of the socket is reset and local and/or
        //!   remote addresses must be specified by comand line arguments or trough open().
        //! - If both strings are not empty, the socket is set in rendezvous mode.
        //! - If only @a local_address is not empty, the socket is set in listener mode.
        //! - If only @a remote_address is not empty, the socket is set in caller mode.
        //! @param [in] local_address Local "[address:]port".
        //! @param [in] remote_address Remote "address:port".
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setAddresses(const UString& local_address, const UString& remote_address, Report& report = CERR)
        {
            return setAddressesInternal(local_address, remote_address, true, report);
        }

        //!
        //! Send a message to the default destination address and port.
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool send(const void* data, size_t size, Report& report = CERR);

        //!
        //! Receive a message.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size, Report& report = CERR);

        //!
        //! Receive a message with timestamp.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [out] timestamp Source timestamp in micro-seconds, negative if not available.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, Report& report = CERR);

        //!
        //! Check if the connection was disconnected by the peer.
        //! This can be used after a send/receive error to differentiate between "end of session" and actual error.
        //! @return True if the connection was closed by the peer.
        //!
        bool peerDisconnected() const;

        //!
        //! Get SRT option.
        //! @param [in] optName Option name as enumeration. The possible values for @a optName are given
        //! by the enumeration type SRT_SOCKOPT in libsrt. The profile of this method uses "int" to remain
        //! portable in the absence of libsrt, but the actual values come from SRT_SOCKOPT in libsrt.
        //! @param [in] optNameStr Option name as ASCII string.
        //! @param [out] optval Address of returned value.
        //! @param [in,out] optlen Size of returned buffer (input), updated to size of returned value.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, Report& report = CERR) const;

        //!
        //! Get the underlying SRT socket handle (use with care).
        //! This method is reserved for low-level operations and should not be used by normal applications.
        //! @return The underlying SRT socket handle.
        //!
        int getSocket() const;

        //!
        //! Check if the SRT socket uses the Message API.
        //! @return True if the SRT socket uses the Message API. False if it uses the buffer API.
        //!
        bool getMessageApi() const;

        //!
        //! Get the version of the SRT library.
        //! @return A string describing the SRT library versions (or the lack of SRT support).
        //!
        static UString GetLibraryVersion();

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts;

        // Internal verson of setAddresses().
        bool setAddressesInternal(const UString& local_address, const UString& remote_address, bool reset, Report& report);
    };
}

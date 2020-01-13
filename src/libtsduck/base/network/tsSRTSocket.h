//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020, Anthony Delannoy
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
#include "tsIPUtils.h"
#include "tsReport.h"
#include "tsArgsSupplierInterface.h"

// Currently, we disable SRT on Windows.
#if defined(TS_WINDOWS) && !defined(TS_NOSRT)
#define TS_NOSRT 1
#endif

#if !defined(TS_NOSRT)
#include <srt/srt.h>

namespace ts {
    //!
    //! SRT socket mode
    //!
    enum SRTSocketMode: int {
        LISTENER   = 0,  //!< Listener mode.
        CALLER     = 1,  //!< Caller mode.
        RENDEZVOUS = 2,  //!< Rendez-vous mode (unsupported).
        LEN        = 3,  //!< Unknown.
    };

    //!
    //! Secure Reliable Transport (SRT) Socket.
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
        //! @param [in] mode SRT socket mode.
        //! @param [in,out] report Where to report error.
        //!
        SRTSocket(SRTSocketMode mode, Report& report = CERR);

        //!
        //! Destructor.
        //!
        ~SRTSocket(void);

        //!
        //! Open the socket.
        //! @param [in] addr Socket address.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(const SocketAddress& addr, Report& report = CERR);

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
        //! Get SRT option.
        //! @param [in] optName Option name as enumeration.
        //! @param [in] optNameStr Option name as ASCII string.
        //! @param [out] optval Address of returned value.
        //! @param [in,out] optlen Size of returned buffer (input), updated to size of returned value.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool getSockOpt(SRT_SOCKOPT optName, const char* optNameStr, void* optval, int& optlen, Report& report = CERR) const;


        //!
        //! Get the underlying socket device handle (use with care).
        //! This method is reserved for low-level operations and should
        //! not be used by normal applications.
        //! @return The underlying socket system device handle or file descriptor.
        //!
        int getSocket() const { return _sock; }

        //!
        //! Check if the SRT socket uses the Message API.
        //! @return True if the SRT socket uses the Message API. False if it uses the buffer API.
        //!
        bool getMessageApi() const { return _messageapi; }

    private:
        bool send(const void* data, size_t size, const SocketAddress& dest, Report& report);

        bool setDefaultAddress(const UString& name, Report& report = CERR);
        bool setDefaultAddress(const SocketAddress& addr, Report& report = CERR);

        bool setSockOpt(SRT_SOCKOPT optName, const char* optNameStr, const void* optval, int optlen, Report& report = CERR);
        bool setSockOptPre(Report& report);
        bool setSockOptPost(Report& report);

        int srtListen(const SocketAddress& addr, Report& report);
        int srtConnect(const SocketAddress& addr, Report& report);

        SocketAddress _default_address;
        SRTSocketMode _mode;
        int _sock;

        // Sock options
        SRT_TRANSTYPE _transtype;
        UString _packet_filter;
        UString _passphrase;
        UString _streamid;
        int _polling_time;
        bool _messageapi;
        int _nakreport;
        int _conn_timeout;
        int _ffs;
        int _linger;
        int _lossmaxttl;
        int _mss;
        int _ohead_bw;
        int _payload_size;
        int _rcvbuf;
        int _sndbuf;
        int _enforce_encryption;
        int32_t _kmrefreshrate;
        int32_t _kmpreannounce;
        int _udp_rcvbuf;
        int _udp_sndbuf;
        int64_t _input_bw;
        int64_t _max_bw;
        int32_t _iptos;
        int32_t _ipttl;
        int32_t _latency;
        int32_t _min_version;
        int32_t _pbkeylen;
        int32_t _peer_idle_timeout;
        int32_t _peer_latency;
        int32_t _rcv_latency;
        int32_t _tlpktdrop;
    };
}

#endif /* TS_NOSRT */

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
//!  SRT Socket
//!
//----------------------------------------------------------------------------

#pragma once

#if !defined(TS_NOSRT)

#include <srt/srt.h>

#include "tsSocketAddress.h"
#include "tsIPUtils.h"
#include "tsReport.h"
#include "tsArgsSupplierInterface.h"

namespace ts {
    enum SRTSocketMode: int {
        LISTENER = 0,
        CALLER = 1,
        RENDEZVOUS = 2,
        LEN = 3,
    };

    //!
    //! SRT Socket.
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
        //! @param [in] auto_open If true, call open() immediately.
        //! @param [in,out] report Where to report error.
        //!
        SRTSocket(enum SRTSocketMode mode, Report& report = CERR);

        //!
        //! Destructor.
        //!
        ~SRTSocket(void);

        // Implementation of Socket interface.
        bool open(const SocketAddress& addr, Report& report = CERR);
        bool close(Report& report = CERR);

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        bool send(const void* data, size_t size, Report& report = CERR);
        bool receive(void* data, size_t max_size, size_t& ret_size, Report& report = CERR);

        // Getters
        bool getSockOpt(SRT_SOCKOPT optName, const char* optNameStr, void* optval, int& optlen, Report& report = CERR) const;
        int getSocket(void) const { return _sock; }
        int getMessageApi(void) const { return _messageapi; }

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
        enum SRTSocketMode _mode;
        int _sock;

        // Sock options
        SRT_TRANSTYPE _transtype;
        UString _packet_filter;
        UString _passphrase;
        UString _streamid;
        int _polling_time;
        int _messageapi;
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

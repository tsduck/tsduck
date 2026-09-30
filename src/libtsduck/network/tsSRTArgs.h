//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Secure Reliable Transport (SRT) socket options.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsURL.h"
#include "tsIPSocketAddress.h"
#include "tsEnumUtils.h"

namespace ts {
    class Args;
    class Report;
    class DuckContext;
}

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) socket mode.
    //!
    enum class SRTSocketMode: int {
        DEFAULT    = -1,  //!< Unspecified, use command line mode.
        LISTENER   =  0,  //!< Listener mode.
        CALLER     =  1,  //!< Caller mode.
        RENDEZVOUS =  2,  //!< Rendez-vous mode.
    };

    //!
    //! Secure Reliable Transport (SRT) transmission mode.
    //!
    enum class SRTTransmissionMode: int {
        DEFAULT = -1,  //!< Unspecified, use command line mode.
        LIVE    =  0,  //!< Live, continue transmission while requesting missing packets.
        FILE    =  1,  //!< File, wait for missing packets.
    };

    //!
    //! Secure Reliable Transport (SRT) statistics mode.
    //! Can be used as bitmask.
    //!
    enum class SRTStatMode: uint16_t {
        NONE     = 0x0000,  //!< Reports nothing.
        RECEIVE  = 0x0001,  //!< Receive statistics (ignored if nothing was received).
        SEND     = 0x0002,  //!< Sender statistics (ignored if nothing was sent).
        TOTAL    = 0x0004,  //!< Statistics since the socket was opened.
        INTERVAL = 0x0008,  //!< Statistics in the last interval (restarted each time it is used).
        ALL      = 0x000F,  //!< Report all statistics.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::SRTStatMode);

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) socket options.
    //! @ingroup libtsduck net
    //!
    //! A negative value in an option means "unspecified value". Options which have default values which
    //! depend on the context are in a std::optional so that no value at is used when unspecified.
    //!
    class TSDUCKDLL SRTArgs
    {
        TS_DEFAULT_COPY_MOVE(SRTArgs);
    public:
        // SRTArgs public options.
        // Boolean and integer types are carefully selected from the libsrt API, modify with care.

        SRTSocketMode          mode = SRTSocketMode::DEFAULT;  //!< SRT socket mode.
        IPSocketAddress        local_address {};               //!< Local socket address (any mode).
        IPSocketAddress        remote_address {};              //!< Remote socket address (caller or rendezvous mode).
        ::linger               linger_opt {0, 0};              //!< Linger time on close.
        int                    backlog = 1;                    //!< Max waiting incoming client in listener mode.

        bool                   live_mode = true;               //!< Live transmission type (i.e. not file transmission type).
        bool                   message_api = true;             //!< Use message API (i.e. not buffer API).
        bool                   reuse_port = true;              //!< Reuse UDP port in listener mode.
        bool                   enforce_encryption = false;     //!< Enforce encrypted communication.
        std::optional<bool>    drift_tracer = false;           //!< Enables or disables time drift tracer. (@@@ TODO: add command line option)
        std::optional<bool>    nakreport = false;              //!< Periodically send NAK reports for missed packets.
        std::optional<bool>    tlpktdrop = false;              //!< Too-late Packet Drop.
        UString                packet_filter {};               //!< Packet filter string.
        UString                passphrase {};                  //!< Passphrase for encryption.
        UString                stream_id {};                   //!< Stream identification string.
        int32_t                fc_packets = -1;                //!< Flow Control limits the maximum number of packets "in flight".
        int32_t                lossmaxttl = -1;                //!< Value (in packets) up to which the Reorder Tolerance may grow.
        int32_t                mss = -1;                       //!< Maximum Segment Size.
        int32_t                ohead_bw = -1;                  //!< Recovery bandwidth overhead above input rate (percent).
        int32_t                payload_size = -1;              //!< Maximum declared size of a single call to sending function in Live mode.
        int32_t                rcvbuf = -1;                    //!< Receive Buffer Size.
        int32_t                sndbuf = -1;                    //!< Send Buffer Size.
        int32_t                udp_rcvbuf = -1;                //!< UDP socket receive buffer size.
        int32_t                udp_sndbuf = -1;                //!< UDP socket send buffer size.
        int32_t                kmrefreshrate = -1;             //!< Interval in packets between Encrypting Keys (SEK).
        int32_t                kmpreannounce = -1;             //!< Interval in packets between a new Stream Encrypting Key (SEK) and key switch.
        int64_t                input_bw = -1;                  //!< Maximum input bandwidth.
        int64_t                max_bw = -1;                    //!< Maximum send bandwidth (-1 means infinite).
        int32_t                iptos = -1;                     //!< IPv4 Type of Service.
        int32_t                ipttl = -1;                     //!< IPv4 Time To Live.
        int32_t                pbkeylen = -1;                  //!< Sender encryption key length, in bytes.
        int32_t                min_version = -1;               //!< Minimum SRT version that is required from the peer.
        std::optional<int32_t> snddropdelay {};                //!< Extra delay before TLPKTDROP is triggered on the data sender.
        cn::milliseconds       connection_timeout = cn::milliseconds(-1); //!< Connection timeout.
        cn::milliseconds       latency = cn::milliseconds(-1);            //!< Configured latency.
        cn::milliseconds       peer_idle_timeout = cn::milliseconds(-1);  //!< Maximum time in [ms] to wait until another packet is received.
        cn::milliseconds       peer_latency = cn::milliseconds(-1);       //!< Latency value provided by the sender side as a minimum value for the receiver.
        cn::milliseconds       rcv_latency = cn::milliseconds(-1);        //!< Latency value in the receiving direction of the socket.

        SRTStatMode            stats_mode = SRTStatMode::ALL;  //!< Amount of statistics to report.
        cn::milliseconds       stats_interval = cn::milliseconds(-1);  //!< If positive, interval between statistics reports.
        bool                   final_stats = false;            //!< Report SRT usage statistics when the SRT socket is closed.
        bool                   json_line = false;              //!< Report statistics in JSON format on one line.
        UString                json_prefix {};                 //!< Noticeable prefix on JSON statistics lines.

        //!
        //! Constructor.
        //!
        SRTArgs() = default;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Set options from an srt: URL.
        //! See TSDuck user guide for a complete description of SRT URLs.
        //! @param [in,out] report Where to report errors.
        //! @param [in] url URL with srt: scheme.
        //! @return True on success, false on error in URL syntax.
        //!
        bool setURL(Report& report, const UString& url) { return setURL(report, URL(url)); }

        //!
        //! Set options from an srt: URL.
        //! See TSDuck user guide for a complete description of SRT URLs.
        //! @param [in] url URL with srt: scheme.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error in URL syntax.
        //!
        bool setURL(Report& report, const URL& url);

        //!
        //! Check if a string is a possible srt: URL.
        //! Only check the URL scheme, not the complete syntax.
        //! @param [in] url URL.
        //! @return True if @a url is a possible srt: URL.
        //!
        static bool IsURL(const UString& url) { return url.starts_with(u"srt://"); }

        //!
        //! Check if an URL is a possible srt: URL.
        //! Only check the URL scheme, not the complete syntax.
        //! @param [in] url URL.
        //! @return True if @a url is a possible srt: URL.
        //!
        static bool IsURL(const URL& url) { return url.getScheme() == u"srt"; }

        //!
        //! Preset local and remote socket addresses in string form.
        //! - If only @a listener is not empty, the socket is set in listener mode.
        //! - If only @a caller is not empty, the socket is set in caller mode.
        //! - If both addresses are not empty, the socket is set in rendezvous mode.
        //! - If both addresses are empty, the current mode of the socket is reset and local and/or
        //!   remote addresses must be specified by command line arguments or through open().
        //! @param [in,out] report Where to report errors.
        //! @param [in] listener Local "[address:]port".
        //! @param [in] caller Remote "address:port".
        //! @param [in] local Optional, can be empty. In caller mode, specify the local outgoing IP address.
        //! @return True on success, false on error.
        //!
        bool setAddresses(Report& report, const IPSocketAddress& listener, const IPSocketAddress& caller, const IPAddress& local = IPAddress())
        {
            return setAddressesInternal(report, listener, caller, local, true);
        }

        //!
        //! Reset to default values.
        //!
        void reset();

    private:
        // Internal verson of setAddresses(). If reset, clear mode and all addresses first.
        bool setAddressesInternal(Report& report, const IPSocketAddress& listener, const IPSocketAddress& caller, const IPAddress& local, bool reset);

        // Temporary values, used when analysing an URL.
        IPAddress _adapter {};
        IPSocketAddress _binder {};
        int     _congestion = -1;
        int32_t _local_port = -1;
        int32_t _ipv6only = -1;
        int32_t _linger_time = -1;
        int32_t _cryptomode = -1;
        int32_t _groupconnect = -1;
        int32_t _retransmitalgo = -1;
        int64_t _min_bw = -1;
    };
}

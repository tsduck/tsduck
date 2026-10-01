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
    enum class SRTSocketMode : int32_t {
        DEFAULT    = -1,  //!< Unspecified, use command line mode.
        LISTENER   =  0,  //!< Listener mode.
        CALLER     =  1,  //!< Caller mode.
        RENDEZVOUS =  2,  //!< Rendez-vous mode.
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

        // Statistics options. These are TSDuck options, not SRT options.
        SRTStatMode            stats_mode = SRTStatMode::ALL;  //!< Amount of statistics to report.
        cn::milliseconds       stats_interval {-1};            //!< If positive, interval between statistics reports.
        bool                   final_stats = false;            //!< Report SRT usage statistics when the SRT socket is closed.
        bool                   json_line = false;              //!< Report statistics in JSON format on one line.
        UString                json_prefix {};                 //!< Noticeable prefix on JSON statistics lines.

        // Network options.
        SRTSocketMode          mode = SRTSocketMode::DEFAULT;  //!< SRT socket mode (SRTO_RENDEZVOUS, SRTO_SENDER, v1.0.4).
        IPSocketAddress        local_address {};               //!< Local socket address (any mode).
        IPSocketAddress        remote_address {};              //!< Remote socket address (caller or rendezvous mode).
        int                    backlog = 1;                    //!< Max waiting incoming client in listener mode.
        ::linger               linger_opt {0, 0};              //!< Linger time on close (SRTO_LINGER).
        bool                   reuse_port = true;              //!< Reuse UDP port in listener mode (SRTO_REUSEADDR).
        int32_t                ipv6_only = -2;                 //!< -1=system default, 0=IPv4 and v6, 1=IPv6 only (SRTO_IPV6ONLY, v1.4.0).
        int32_t                iptos = -1;                     //!< IPv4 Type of Service (SRTO_IPTOS, v1.0.5).
        int32_t                ipttl = -1;                     //!< IPv4 Time To Live (SRTO_IPTTL, v1.0.5).
        int32_t                mss = -1;                       //!< Maximum Segment Size (SRTO_MSS).
        int32_t                udp_rcvbuf = -1;                //!< UDP socket receive buffer size (SRTO_UDP_RCVBUF).
        int32_t                udp_sndbuf = -1;                //!< UDP socket send buffer size (SRTO_UDP_SNDBUF).

        // Transmission mode (live vs. file) and dependent parameters.
        // Their default values depend on the transmisson mode.
        // Don't set the dependent parameters unless you know what you are doing.
        bool                   live_mode = true;               //!< Live transmission type, not file (SRTO_TRANSTYPE, v1.3.0).
        UString                congestion {};                  //!< Congestion controller, "live", "file" (SRTO_CONGESTION, v1.3.0).
        std::optional<bool>    message_api {};                 //!< Use message API, not buffer API (SRTO_MESSAGEAPI, v1.3.0).
        std::optional<bool>    nakreport {};                   //!< Periodically send NAK reports for missed packets (SRTO_NAKREPORT, v1.1.0).
        cn::milliseconds       rcv_latency {-1};               //!< Latency value in the receiving direction of the socket (SRTO_RCVLATENCY, v1.3.0).
        std::optional<bool>    tlpktdrop {};                   //!< Too-late Packet Drop (SRTO_TLPKTDROP, v1.0.6).
        std::optional<bool>    tsbpdmode {};                   //!< Use Timestamp-based Packet Delivery mode (SRTO_TSBPDMODE).

        // Group connection options.
        int32_t                group_connect = -1;             //!< Listener accept group connection, 0 or 1 (SRTO_GROUPCONNECT, v1.5.0).
        cn::milliseconds       groupminstabletimeo {-1};       //!< Group minimum stability timeout (SRTO_GROUPMINSTABLETIMEO, v1.5.0).

        // Bandwidth options.
        int64_t                input_bw = -1;                  //!< Maximum input bandwidth (SRTO_INPUTBW, v1.0.5).
        int64_t                min_input_bw = -1;              //!< Minimum allowed input bandwidth (SRTO_MININPUTBW, v1.4.3).
        int64_t                max_bw = -2;                    //!< Maximum send bandwidth, -1 means infinite (SRTO_MAXBW, v1.0.5).
        int64_t                max_rexmit_bw = -2;             //!< Maximum bandwidth for retransmission, -1 means infinite (SRTO_MAXREXMITBW, v1.5.3).
        int32_t                ohead_bw = -1;                  //!< Recovery bandwidth overhead above input rate, in percent (SRTO_OHEADBW, v1.0.5).

        // Encryption options.
        // SEK = Stream Encrypting Key.
        UString                passphrase {};                  //!< Passphrase for encryption (SRTO_PASSPHRASE).
        int32_t                crypto_mode = -1;               //!< Encryption mode, 0=negotiate, 1=AES-CTR, 2=AES-GCM (SRTO_CRYPTOMODE, v1.5.2)
        int32_t                pbkeylen = -1;                  //!< Sender encryption key length, in bytes, 0=default, 16, 24, 32 (SRTO_PBKEYLEN).
        std::optional<bool>    enforce_encryption {};          //!< Enforce same encryption (SRTO_ENFORCEDENCRYPTION, v1.3.2).
        int32_t                kmrefreshrate = -1;             //!< Interval in packets between SEK (SRTO_KMREFRESHRATE, 1.3.2).
        int32_t                kmpreannounce = -1;             //!< Interval in packets between new SEK and SEK switch (SRTO_KMPREANNOUNCE, v1.3.2).

        // Other SRT options.
        std::optional<bool>    drift_tracer {};                //!< Enables or disables time drift tracer (SRTO_DRIFTTRACER, v1.4.2).
        UString                packet_filter {};               //!< Packet filter string (SRTO_PACKETFILTER, v1.4.0).
        UString                stream_id {};                   //!< Stream identification string (SRTO_STREAMID, v1.3.0).
        int32_t                fc_packets = -1;                //!< Flow Control, limit max packets "in flight" (SRTO_FC).
        int32_t                lossmaxttl = -1;                //!< Value (in packets) up to which the Reorder Tolerance may grow (SRTO_LOSSMAXTTL, v1.2.0).
        int32_t                payload_size = -1;              //!< Maximum size of a single send in Live mode (SRTO_PAYLOADSIZE, v1.3.0).
        int32_t                rcvbuf = -1;                    //!< Receive Buffer Size, in bytes (SRTO_RCVBUF).
        int32_t                sndbuf = -1;                    //!< Send Buffer Size (SRTO_SNDBUF).
        int32_t                min_version = -1;               //!< Minimum SRT version that is required from the peer (SRTO_MINVERSION, v1.3.0).
        int32_t                retransmit_algo = -1;           //!< Choose between retransmission algorithms (SRTO_RETRANSMITALGO, v1.4.2).
        cn::milliseconds       snd_drop_delay {-2};            //!< Extra delay before TLPKTDROP, -1 = do not drop packets (SRTO_SNDDROPDELAY, v1.3.2).
        cn::milliseconds       connection_timeout {-1};        //!< Connection timeout (SRTO_CONNTIMEO, v1.1.2).
        cn::milliseconds       latency {-1};                   //!< Configured latency, set both SRTO_RCVLATENCY and SRTO_PEERLATENCY (SRTO_LATENCY, v1.0.2).
        cn::milliseconds       peer_idle_timeout {-1};         //!< Max time to wait until another packet is received (SRTO_PEERIDLETIMEO, v1.3.3).
        cn::milliseconds       peer_latency {-1};              //!< Latency provided by sender as min value for the receiver (SRTO_PEERLATENCY, v1.3.0).

        // Unimplemented SRT options, read-only or considered useless or too exotic:
        // - SRTO_BINDTODEVICE, v1.3.0
        // - SRTO_EVENT, read-only, with fake-epoll
        // - SRTO_GROUPTYPE, v1.5.0, read-only
        // - SRTO_ISN, v1.3.0
        // - SRTO_KMSTATE, v1.0.2
        // - SRTO_PEERVERSION, v1.1.0, read-only
        // - SRTO_RCVDATA, read-only
        // - SRTO_RCVKMSTATE, v1.2.0, read-only
        // - SRTO_RCVSYN, with fake-epoll
        // - SRTO_RCVTIMEO, with fake-epoll
        // - SRTO_SNDDATA, read-only
        // - SRTO_SNDKMSTATE, v1.2.0, read-only
        // - SRTO_SNDSYN, with fake-epoll
        // - SRTO_SNDTIMEO, with fake-epoll
        // - SRTO_STATE, read-only
        // - SRTO_VERSION, read-only

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
        //! Set the minimum version field from a "x.y.z" string.
        //! @param [in,out] report Where to report errors.
        //! @param [in] version Version string in "x.y.z" format.
        //! Ignored if empty (@a min_version is left unmodified).
        //! @return True on success, false on error in version syntax.
        //!
        bool setMinVersion(Report& report, const UString& version);

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
        static bool IsSRTURL(const UString& url) { return url.starts_with(u"srt://"); }

        //!
        //! Check if an URL is a possible srt: URL.
        //! Only check the URL scheme, not the complete syntax.
        //! @param [in] url URL.
        //! @return True if @a url is a possible srt: URL.
        //!
        static bool IsSRTURL(const URL& url) { return url.getScheme() == u"srt"; }

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
        UString         _min_version {};
        IPAddress       _adapter {};
        IPSocketAddress _binder {};
        int32_t         _local_port = -1;
        int32_t         _linger_time = -1;
    };
}

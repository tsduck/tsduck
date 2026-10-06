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
        // SRT options with std::optional type are configured in the socket only when their value is set.

        // Statistics options. These are TSDuck options, not SRT options.
        SRTStatMode            stats_mode = SRTStatMode::ALL;  //!< Amount of statistics to report.
        cn::milliseconds       stats_interval {-1};            //!< If positive, interval between statistics reports.
        bool                   final_stats = false;            //!< Report SRT usage statistics when the SRT socket is closed.
        bool                   json_line = false;              //!< Report statistics in JSON format on one line.
        UString                json_prefix {};                 //!< Noticeable prefix on JSON statistics lines.

        // Network options.
        SRTSocketMode          mode = SRTSocketMode::DEFAULT;  //!< SRT socket mode (SRTO_RENDEZVOUS).
        IPSocketAddress        local_address {};               //!< Local socket address (any mode).
        IPSocketAddress        remote_address {};              //!< Remote socket address (caller or rendezvous mode).
        int                    backlog = 1;                    //!< Max waiting incoming client in listener mode.
        ::linger               linger_opt {0, 0};              //!< Linger time on close (SRTO_LINGER).
        bool                   reuse_port = true;              //!< Reuse UDP port in listener mode (SRTO_REUSEADDR).
        std::optional<bool>    sender {};                      //!< Indicate that the socket acts as sender, hint only (SRTO_SENDER, v1.0.4)
        std::optional<int32_t> ipv6_only {};                   //!< -1=system default, 0=IPv4 and v6, 1=IPv6 only (SRTO_IPV6ONLY, v1.4.0).
        std::optional<int32_t> iptos {};                       //!< IPv4 Type of Service (SRTO_IPTOS, v1.0.5).
        std::optional<int32_t> ipttl {};                       //!< IPv4 Time To Live (SRTO_IPTTL, v1.0.5).
        std::optional<int32_t> mss {};                         //!< Maximum Segment Size (SRTO_MSS).
        std::optional<int32_t> udp_rcvbuf {};                  //!< UDP socket receive buffer size (SRTO_UDP_RCVBUF).
        std::optional<int32_t> udp_sndbuf {};                  //!< UDP socket send buffer size (SRTO_UDP_SNDBUF).

        // Transmission mode (live vs. file) and dependent parameters.
        // Their default values depend on the transmisson mode.
        // Don't set the dependent parameters unless you know what you are doing.
        std::optional<bool>    live_mode {};                   //!< Live transmission type, not file (SRTO_TRANSTYPE, v1.3.0).
        std::optional<UString> congestion {};                  //!< Congestion controller, "live", "file" (SRTO_CONGESTION, v1.3.0).
        std::optional<bool>    message_api {};                 //!< Use message API, not buffer API (SRTO_MESSAGEAPI, v1.3.0).
        std::optional<bool>    nakreport {};                   //!< Periodically send NAK reports for missed packets (SRTO_NAKREPORT, v1.1.0).
        std::optional<bool>    tlpktdrop {};                   //!< Too-late Packet Drop (SRTO_TLPKTDROP, v1.0.6).
        std::optional<bool>    tsbpdmode {};                   //!< Use Timestamp-based Packet Delivery mode (SRTO_TSBPDMODE).

        // Group connection options.
        std::optional<int32_t> group_connect {};               //!< Listener accept group connection, 0 or 1 (SRTO_GROUPCONNECT, v1.5.0).
        std::optional<cn::milliseconds> groupminstabletimeo {};//!< Group minimum stability timeout (SRTO_GROUPMINSTABLETIMEO, v1.5.0).

        // Bandwidth options.
        std::optional<int64_t> input_bw {};                    //!< Maximum input bandwidth (SRTO_INPUTBW, v1.0.5).
        std::optional<int64_t> min_input_bw {};                //!< Minimum allowed input bandwidth (SRTO_MININPUTBW, v1.4.3).
        std::optional<int64_t> max_bw {};                      //!< Maximum send bandwidth, -1 means infinite (SRTO_MAXBW, v1.0.5).
        std::optional<int64_t> max_rexmit_bw {};               //!< Maximum bandwidth for retransmission, -1 means infinite (SRTO_MAXREXMITBW, v1.5.3).
        std::optional<int32_t> ohead_bw {};                    //!< Recovery bandwidth overhead above input rate, in percent (SRTO_OHEADBW, v1.0.5).

        // Encryption options.
        // SEK = Stream Encrypting Key.
        std::optional<UString> passphrase {};                  //!< Passphrase for encryption (SRTO_PASSPHRASE).
        std::optional<int32_t> crypto_mode {};                 //!< Encryption mode, 0=negotiate, 1=AES-CTR, 2=AES-GCM (SRTO_CRYPTOMODE, v1.5.2)
        std::optional<int32_t> pbkeylen {};                    //!< Sender encryption key length, in bytes, 0=default, 16, 24, 32 (SRTO_PBKEYLEN).
        std::optional<bool>    enforce_encryption {};          //!< Enforce same encryption (SRTO_ENFORCEDENCRYPTION, v1.3.2).
        std::optional<int32_t> kmrefreshrate {};               //!< Interval in packets between SEK (SRTO_KMREFRESHRATE, 1.3.2).
        std::optional<int32_t> kmpreannounce {};               //!< Interval in packets between new SEK and SEK switch (SRTO_KMPREANNOUNCE, v1.3.2).

        // Other SRT options.
        std::optional<bool>    drift_tracer {};                //!< Enables or disables time drift tracer (SRTO_DRIFTTRACER, v1.4.2).
        std::optional<UString> packet_filter {};               //!< Packet filter string (SRTO_PACKETFILTER, v1.4.0).
        std::optional<UString> stream_id {};                   //!< Stream identification string (SRTO_STREAMID, v1.3.0).
        std::optional<int32_t> fc_packets {};                  //!< Flow Control, limit max packets "in flight" (SRTO_FC).
        std::optional<int32_t> lossmaxttl {};                  //!< Value (in packets) up to which the Reorder Tolerance may grow (SRTO_LOSSMAXTTL, v1.2.0).
        std::optional<int32_t> payload_size {};                //!< Maximum size of a single send in Live mode (SRTO_PAYLOADSIZE, v1.3.0).
        std::optional<int32_t> rcvbuf {};                      //!< Receive Buffer Size, in bytes (SRTO_RCVBUF).
        std::optional<int32_t> sndbuf {};                      //!< Send Buffer Size (SRTO_SNDBUF).
        std::optional<int32_t> min_version {};                 //!< Minimum SRT version that is required from the peer (SRTO_MINVERSION, v1.3.0).
        std::optional<int32_t> retransmit_algo {};             //!< Choose between retransmission algorithms (SRTO_RETRANSMITALGO, v1.4.2).
        std::optional<cn::milliseconds> rcv_latency {};        //!< Latency value in the receiving direction of the socket (SRTO_RCVLATENCY, v1.3.0).
        std::optional<cn::milliseconds> snd_drop_delay {};     //!< Extra delay before TLPKTDROP, -1 = do not drop packets (SRTO_SNDDROPDELAY, v1.3.2).
        std::optional<cn::milliseconds> connection_timeout {}; //!< Connection timeout (SRTO_CONNTIMEO, v1.1.2).
        std::optional<cn::milliseconds> latency {};            //!< Configured latency, set both SRTO_RCVLATENCY and SRTO_PEERLATENCY (SRTO_LATENCY, v1.0.2).
        std::optional<cn::milliseconds> peer_idle_timeout {};  //!< Max time to wait until another packet is received (SRTO_PEERIDLETIMEO, v1.3.3).
        std::optional<cn::milliseconds> peer_latency {};       //!< Latency provided by sender as min value for the receiver (SRTO_PEERLATENCY, v1.3.0).

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
        //! Rebuild the minimum version field as a "x.y.z" string.
        //! @return The minimum version string, or the empty string if not set.
        //!
        UString minVersionString() const;

        //!
        //! Set options from an srt: URL.
        //! See the TSDuck user guide for a complete description of SRT URLs.
        //! @see https://github.com/Haivision/srt/blob/master/docs/apps/srt-live-transmit.md#medium-srt
        //! @param [in,out] report Where to report errors.
        //! @param [in] url URL with srt: scheme.
        //! @return True on success, false on error in URL syntax.
        //!
        bool setURL(Report& report, const UString& url) { return setURL(report, URL(url)); }

        //!
        //! Set options from an srt: URL.
        //! See the TSDuck user guide for a complete description of SRT URLs.
        //! @see https://github.com/Haivision/srt/blob/master/docs/apps/srt-live-transmit.md#medium-srt
        //! @param [in] url URL with srt: scheme.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error in URL syntax.
        //!
        bool setURL(Report& report, const URL& url);

        //!
        //! Rebuild a srt:// URL from the set of SRT parameters.
        //! See the TSDuck user guide for a complete description of SRT URLs.
        //! @see https://github.com/Haivision/srt/blob/master/docs/apps/srt-live-transmit.md#medium-srt
        //! @param [in] standard_only When true (the default), only set standard parameter from the Haivision description.
        //! When false, add non-standard and undocumented additional parameters from TSDuck implementation.
        //! @return The resulting URL.
        //!
        UString toURL(bool standard_only = true) const;

        //!
        //! Rebuild a srt:// URL from the set of SRT parameters.
        //! See the TSDuck user guide for a complete description of SRT URLs.
        //! @see https://github.com/Haivision/srt/blob/master/docs/apps/srt-live-transmit.md#medium-srt
        //! @param [out] url The resulting URL.
        //! @param [in] standard_only When true (the default), only set standard parameter from the Haivision description.
        //! When false, add non-standard and undocumented additional parameters from TSDuck implementation.
        //!
        void toURL(URL& url, bool standard_only = true) const;

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
        // Internal version of setAddresses(). If reset, clear mode and all addresses first.
        bool setAddressesInternal(Report& report, const IPSocketAddress& listener, const IPSocketAddress& caller, const IPAddress& local, bool reset);

        // Temporary values, used when analysing or building an URL.
        mutable IPAddress              _adapter {};
        mutable IPSocketAddress        _binder {};
        mutable std::optional<UString> _min_version {};
        mutable std::optional<int32_t> _local_port {};
        mutable std::optional<int32_t> _linger_time {};

        // Store enumeration values as if they were int32_t.
        using I32ENU = int32_t SRTArgs::*;
        static_assert(sizeof(SRTArgs::mode) == sizeof(int32_t));

        // Definition of parameters in URL query string.
        struct Param {
            // Field to update in SRTArgs.
            std::optional<bool>             SRTArgs::* bl  = nullptr;
            std::optional<int32_t>          SRTArgs::* i32 = nullptr;
            std::optional<int64_t>          SRTArgs::* i64 = nullptr;
            std::optional<cn::milliseconds> SRTArgs::* ms  = nullptr;
            std::optional<UString>          SRTArgs::* str = nullptr;
            int32_t                         SRTArgs::* enu = nullptr; // enum with int32_t representation
            IPAddress                       SRTArgs::* ip  = nullptr;
            IPSocketAddress                 SRTArgs::* sok = nullptr;

            // Value boundaries or list.
            int64_t      min = 0;
            int64_t      max = std::numeric_limits<int64_t>::max();
            const Names* names = nullptr;
            bool         standard = true;
        };

        // Map of query parameter name to description.
        using QueryParameterMap = std::map<UString, Param>;

        // Define all possible parameters in the URL query string.
        static const QueryParameterMap& QueryParameters();
    };
}

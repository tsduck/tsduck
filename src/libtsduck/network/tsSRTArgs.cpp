//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSRTArgs.h"
#include "tsArgs.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Reset to default values.
//----------------------------------------------------------------------------

void ts::SRTArgs::reset()
{
    *this = SRTArgs();
}


//----------------------------------------------------------------------------
// Add command line option definitions in an Args.
//----------------------------------------------------------------------------

void ts::SRTArgs::defineArgs(Args& args)
{
    args.option(u"", 0, Args::STRING, 0, 1);
    args.help(u"", u"URL",
              u"Optional srt:// URL which specifies all or most parameters. "
              u"The parameters in the URL overwrite specific options in case of conflict.");

    args.option(u"backlog", 0, Args::POSITIVE);
    args.help(u"backlog",
              u"With --listener, specify the number of allowed waiting incoming clients. "
              u"The default is one.");

    args.option(u"bufferapi");
    args.help(u"bufferapi", u"When set, this socket uses the Buffer API. The default is Message API.");

    args.option(u"caller", 'c', Args::IPSOCKADDR);
    args.help(u"caller",
              u"Use SRT in caller (or rendezvous) mode. "
              u"The parameter specifies the IP remote address (or host name) and UDP port. "
              u"If --listener is also specified, the SRT socket works in rendezvous mode.");

    args.option<cn::milliseconds>(u"conn-timeout");
    args.help(u"conn-timeout",
              u"Connect timeout, in milliseconds. "
              u"SRT cannot connect for RTT > 1500 msec (2 handshake exchanges) with the default connect timeout of 3 seconds. "
              u"This option applies to the caller and rendezvous connection modes. "
              u"The connect timeout is 10 times the value set for the rendezvous mode "
              u"(which can be used as a workaround for this connection problem with earlier versions).");

    args.option(u"enforce-encryption");
    args.help(u"enforce-encryption",
              u"This option enforces that both connection parties have the same passphrase set "
              u"(including empty, that is, with no encryption), or otherwise the connection is rejected.");

    args.option(u"fc", 0, Args::POSITIVE);
    args.help(u"fc", u"packets",
              u"Flow Control limits the maximum number of packets \"in flight\" (that can be sent without being acknowledged).");

    args.option(u"final-statistics");
    args.help(u"final-statistics",
              u"Report SRT usage statistics when the SRT socket is closed. "
              u"This option is implicit with --statistics-interval.");

    args.option(u"input-bw", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int64_t>::max());
    args.help(u"input-bw",
              u"This option is effective only if SRTO_MAXBW is set to 0 (relative). It controls "
              u"the maximum bandwidth together with SRTO_OHEADBW option according to the formula: "
              u"MAXBW = INPUTBW * (100 + OHEADBW) / 100. "
              u"When this option is set to 0 (automatic) then the real INPUTBW value will be "
              u"estimated from the rate of the input (cases when the application calls the srt_send* function) "
              u"during transmission."
              u"Recommended: set this option to the predicted bitrate of your live stream and keep default 25% "
              u"value for SRTO_OHEADBW.");

    args.option(u"iptos", 0, Args::INTEGER, 0, 1, 0, 255);
    args.help(u"iptos",
              u"IPv4 Type of Service (see IP_TOS option for IP) or IPv6 Traffic Class "
              u"(see IPV6_TCLASS of IPv6) depending on socket address family. Applies to sender only. "
              u"Sender: user configurable, default: 0xB8.");

    args.option(u"ipttl", 0, Args::INTEGER, 0, 1, 1, 255);
    args.help(u"ipttl",
              u"IPv4 Time To Live (see IP_TTL option for IP) or IPv6 unicast hops "
              u"(see IPV6_UNICAST_HOPS for IPV6) depending on socket address family. "
              u"Applies to sender only, default: 64.");

    args.option(u"json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"json-line", u"'prefix'",
              u"With --statistics-interval or --final-statistics, report the statistics as one single line in JSON format. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");

    args.option(u"kmpreannounce", 0, Args::INTEGER, 0, 1, 1, std::numeric_limits<int32_t>::max());
    args.help(u"kmpreannounce", u"packets",
              u"The interval (defined in packets) between when a new Stream Encrypting Key (SEK) "
              u"is sent and when switchover occurs. This value also applies to the subsequent "
              u"interval between when switchover occurs and when the old SEK is decommissioned. "
              u"Note: The allowed range for this value is between 1 and half of the current value "
              u"of SRTO_KMREFRESHRATE. The minimum value should never be less than the flight "
              u"window (i.e. the number of packets that have already left the sender but have "
              u"not yet arrived at the receiver).");

    args.option(u"kmrefreshrate", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"kmrefreshrate", u"packets",
              u"The number of packets to be transmitted after which the Stream Encryption Key (SEK), "
              u"used to encrypt packets, will be switched to the new one. Note that the old and new "
              u"keys live in parallel for a certain period of time (see SRTO_KMPREANNOUNCE) before "
              u"and after the switchover.");

    args.option<cn::milliseconds>(u"latency");
    args.help(u"latency",
              u"This flag sets both SRTO_RCVLATENCY and SRTO_PEERLATENCY to the same value. "
              u"Note that prior to version 1.3.0 this is the only flag to set the latency, "
              u"however this is effectively equivalent to setting SRTO_PEERLATENCY, when the "
              u"side is sender (see SRTO_SENDER) and SRTO_RCVLATENCY when the side is receiver, "
              u"and the bidirectional stream sending in version 1.2.0 is not supported.");

    args.option(u"linger", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"linger", u"seconds",
              u"Linger time on close. Define how long, in seconds, to enable queued "
              u"data to be sent after end of stream. Default: no linger.");

    args.option(u"listener", 'l', Args::IPSOCKADDR_OA);
    args.help(u"listener",
              u"Use SRT in listener (or rendezvous) mode. "
              u"The parameter specifies the IP local address and UDP port on which the SRT socket listens. "
              u"The address is optional, the port is mandatory. "
              u"If --caller is also specified, the SRT socket works in rendezvous mode.");

    args.option(u"local-interface", 0, Args::IPADDR);
    args.help(u"local-interface",
              u"In caller mode, use the specified local IP interface for outgoing connections. "
              u"This option is incompatible with --listener.");

    args.option(u"lossmaxttl", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"lossmaxttl",
              u"The value up to which the Reorder Tolerance may grow. When Reorder Tolerance is > 0, "
              u"then packet loss report is delayed until that number of packets come in. "
              u"Reorder Tolerance increases every time a 'belated' packet has come, but it wasn't due "
              u"to retransmission (that is, when UDP packets tend to come out of order), with the "
              u"difference between the latest sequence and this packet's sequence, and not more "
              u"than the value of this option. By default it's 0, which means that this mechanism "
              u"is turned off, and the loss report is always sent immediately upon "
              u"experiencing a 'gap' in sequences.");

    args.option(u"max-bw", 0, Args::INTEGER, 0, 1, -1, std::numeric_limits<int64_t>::max());
    args.help(u"max-bw",
              u"Maximum send bandwidth. NOTE: This option has a default value of -1. "
              u"Although in case when the stream rate is mostly constant it is recommended to "
              u"use value 0 here and shape the bandwidth limit using SRTO_INPUTBW "
              u"and SRTO_OHEADBW options.");

    args.option(u"messageapi");
    args.help(u"messageapi", u"Use the Message API. This is now the default, use --bufferapi to disable it.");

    args.option(u"min-version", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"min-version",
              u"The minimum SRT version that is required from the peer, in the format 0x010203 for v1.2.3 for instance. "
              u"A connection to a peer that does not satisfy the minimum version requirement will be rejected.");

    args.option(u"mss", 0, Args::INTEGER, 0, 1, 76, std::numeric_limits<int32_t>::max());
    args.help(u"mss", u"bytes",
              u"Maximum Segment Size. Used for buffer allocation and rate calculation using "
              u"packet counter assuming fully filled packets. The smallest MSS between the "
              u"peers is used. This is 1500 by default in the overall internet. This is "
              u"the maximum size of the UDP packet and can be only decreased, unless you "
              u"have some unusual dedicated network settings. Not to be mistaken with the "
              u"size of the UDP payload or SRT payload - this size is the size of the IP "
              u"packet, including the UDP and SRT headers.");

    args.option(u"nakreport", 0, Args::BOOLEAN, 0, 0, 0, 0, true);
    args.help(u"nakreport",
              u"When set to true or specified without a value, every report for a detected loss will be repeated. "
              u"The default is true for Live mode, and false for File mode.");

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"With --listener, disable the reuse port socket option. "
              u"Do not use unless completely necessary.");

    args.option(u"ohead-bw", 0, Args::INTEGER, 0, 1, 5, 100);
    args.help(u"ohead-bw", u"percent",
              u"Recovery bandwidth overhead above input rate (see SRTO_INPUTBW). "
              u"It is effective only if SRTO_MAXBW is set to 0.");

    args.option(u"packet-filter", 0, Args::STRING);
    args.help(u"packet-filter",
              u"Set up the packet filter. The string must match appropriate syntax for packet filter setup."
              u"See: https://github.com/Haivision/srt/blob/master/docs/packet-filtering-and-fec.md");

    args.option(u"passphrase", 0, Args::STRING);
    args.help(u"passphrase",
              u"Sets the passphrase for encryption. This turns encryption on on this side (or turns "
              u"it off, if empty passphrase is passed).");

    args.option(u"payload-size", 0, Args::INTEGER, 0, 1, 0, 1456);
    args.help(u"payload-size",
              u"Sets the maximum declared size of a single call to sending function in Live mode. "
              u"Use 0 if this value isn't used (which is default in file mode). This value shall "
              u"not be exceeded for a single data sending instruction in Live mode.");

    args.option(u"pbkeylen", 0, Args::INTEGER, 0, 1, 0, 32);
    args.help(u"pbkeylen", u"bytes",
              u"Sender encryption key length, can be 0, 16 (AES-128), 24 (AES-192), 32 (AES-256).");

    args.option<cn::milliseconds>(u"peer-idle-timeout");
    args.help(u"peer-idle-timeout",
              u"The maximum time in [ms] to wait until any packet is received from peer since "
              u"the last such packet reception. If this time is passed, connection is considered "
              u"broken on timeout.");

    args.option<cn::milliseconds>(u"peer-latency");
    args.help(u"peer-latency",
              u"The latency value (as described in SRTO_RCVLATENCY) that is set by the sender "
              u"side as a minimum value for the receiver.");

    args.option(u"rcvbuf", 0, Args::POSITIVE);
    args.help(u"rcvbuf", u"bytes", u"Receive Buffer Size.");

    args.option<cn::milliseconds>(u"rcv-latency");
    args.help(u"rcv-latency",
              u"The time that should elapse since the moment when the packet was sent and "
              u"the moment when it's delivered to the receiver application in the receiving function.");

    args.option(u"sndbuf", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"sndbuf", u"bytes",
              u"Send Buffer Size. Warning: configured in bytes, converted in packets, "
              u"when set, based on MSS value. For desired result, configure MSS first.");

    args.option(u"snddropdelay", 0, Args::INTEGER, 0, 1, -1, std::numeric_limits<int32_t>::max());
    args.help(u"snddropdelay",
              u"Sets an extra delay, in milliseconds, before --tlpktdrop is triggered on the data sender. "
              u"This delay is added to the default drop delay time interval value. "
              u"Keep in mind that the longer the delay, the more probable it becomes that packets would be "
              u"retransmitted uselessly because they will be dropped by the receiver anyway. "
              u"Option --tlpktdrop discards packets reported as lost if it is already too late to send them "
              u"(the receiver would discard them even if received). "
              u"With the special value -1, do not drop packets on the sender at all (retransmit them always when requested). "
              u"The default is 0 in live mode and -1 in file mode.");

    args.option<cn::milliseconds>(u"statistics-interval");
    args.help(u"statistics-interval",
              u"Report SRT usage statistics at regular intervals, in milliseconds. "
              u"The specified interval is a minimum value, actual reporting can occur "
              u"only when data are exchanged over the SRT socket.");

    args.option(u"streamid", 0, Args::STRING);
    args.help(u"streamid",
              u"A string limited to 512 characters that can be set on the socket prior to connecting. "
              u"This stream ID will be able to be retrieved by the listener side from the socket that "
              u"is returned from srt_accept and was connected by a socket with that set stream ID (so "
              u"you usually use SET on the socket used for srt_connect and GET on the socket retrieved "
              u"from srt_accept). This string can be used completely free-form, however it's highly "
              u"recommended to follow the SRT Access Control guidelines.");

    args.option(u"tlpktdrop", 0, Args::BOOLEAN, 0, 0, 0, 0, true);
    args.help(u"tlpktdrop",
              u"Too-late Packet Drop. When enabled on receiver, it skips missing packets that "
              u"have not been delivered in time and delivers the subsequent packets to the "
              u"application when their time-to-play has come. It also sends a fake ACK to the sender. "
              u"When enabled on sender and enabled on the receiving peer, sender drops the older "
              u"packets that have no chance to be delivered in time. It is automatically enabled "
              u"in sender if receiver supports it. The default is true in Live mode, false in File mode.");

    static const Names transtype_names({
        {u"file", false},
        {u"live", true},
    });
    args.option(u"transtype", 0, transtype_names);
    args.help(u"transtype",
              u"Sets the SRT transmission type for the socket. "
              u"The default is live (continue transmission while requesting missed packets).");

    args.option(u"udp-rcvbuf", 0, Args::POSITIVE);
    args.help(u"udp-rcvbuf", u"bytes", u"UDP socket receive buffer size in bytes.");

    args.option(u"udp-sndbuf", 0, Args::POSITIVE);
    args.help(u"udp-sndbuf", u"bytes", u"UDP socket send buffer size in bytes.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::SRTArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool success = true;

    // Resolve caller/listener/rendezvous addresses.
    IPSocketAddress listener;
    IPSocketAddress caller;
    IPAddress local;
    args.getSocketValue(listener, u"listener");
    args.getSocketValue(caller, u"caller");
    args.getIPValue(local, u"local-interface");

    if (!setAddressesInternal(args, listener, caller, local, false)) {
        return false;
    }

    // Transmission type is live by default. Enum value must be 0 or 1.
    const auto transtype = args.intValue<std::uintmax_t>(u"transtype", true);
    live_mode = transtype != 0;
    if (transtype > 1) {
        args.error(u"invalid --transtype value: %s", args.value(u"transtype"));
        success = false;
    }

    // --messageapi is now the default.
    message_api = !args.present(u"bufferapi");
    if (args.present(u"bufferapi") && args.present(u"messageapi")) {
        args.error(u"--bufferapi and --messageapi are mutually exclusive");
        success = false;
    }

    reuse_port = !args.present(u"no-reuse-port");
    args.getIntValue(backlog, u"backlog", 1);
    linger_opt.l_onoff = args.present(u"linger");
    args.getIntValue(linger_opt.l_linger, u"linger");

    enforce_encryption = args.present(u"enforce-encryption");
    args.getOptionalBoolValue(nakreport, u"nakreport");
    args.getOptionalBoolValue(tlpktdrop, u"tlpktdrop");
    args.getOptionalIntValue(snddropdelay, u"snddropdelay");
    args.getChronoValue(connection_timeout, u"conn-timeout", cn::milliseconds(-1));
    args.getIntValue(fc_packets, u"fc", -1);
    args.getIntValue(input_bw, u"input-bw", -1);
    args.getIntValue(iptos, u"iptos", -1);
    args.getIntValue(ipttl, u"ipttl", -1);
    args.getIntValue(kmrefreshrate, u"kmrefreshrate", -1);
    args.getIntValue(kmpreannounce, u"kmpreannounce", -1);
    args.getIntValue(lossmaxttl, u"lossmaxttl", -1);
    args.getIntValue(max_bw, u"max-bw", -1);
    args.getIntValue(min_version, u"min-version", -1);
    args.getIntValue(mss, u"mss", -1);
    args.getIntValue(ohead_bw, u"ohead-bw", -1);
    args.getValue(stream_id, u"streamid");
    args.getValue(packet_filter, u"packet-filter");
    args.getValue(passphrase, u"passphrase");
    args.getIntValue(payload_size, u"payload-size", -1);
    args.getIntValue(pbkeylen, u"pbkeylen", -1);
    args.getChronoValue(latency, u"latency", cn::milliseconds(-1));
    args.getChronoValue(peer_idle_timeout, u"peer-idle-timeout", cn::milliseconds(-1));
    args.getChronoValue(peer_latency, u"peer-latency", cn::milliseconds(-1));
    args.getChronoValue(rcv_latency, u"rcv-latency", cn::milliseconds(-1));
    args.getIntValue(rcvbuf, u"rcvbuf", -1);
    args.getIntValue(sndbuf, u"sndbuf", -1);
    args.getIntValue(udp_rcvbuf, u"udp-rcvbuf", -1);
    args.getIntValue(udp_sndbuf, u"udp-sndbuf", -1);

    args.getChronoValue(stats_interval, u"statistics-interval");
    final_stats = stats_interval > cn::milliseconds::zero() || args.present(u"final-statistics");
    json_line = args.present(u"json-line");
    args.getValue(json_prefix, u"json-line");

    // The URL overwrites any previous option.
    const UString url(args.value(u""));
    if (!url.empty()) {
        success = setURL(args, url) && success;
    }

    return success;
}


//----------------------------------------------------------------------------
// Preset local and remote socket addresses in string form.
//----------------------------------------------------------------------------

bool ts::SRTArgs::setAddressesInternal(Report& report, const IPSocketAddress& listener, const IPSocketAddress& caller, const IPAddress& local, bool reset)
{
    // Reset the addresses if needed.
    if (reset) {
        mode = SRTSocketMode::DEFAULT;
        local_address.clear();
        remote_address.clear();
    }

    // Nothing more than reset when neither listener nor caller are specified.
    if (!caller.hasPort() && !listener.hasPort()) {
        return true;
    }

    // Resolve communication mode.
    if (!caller.hasAddress() || !caller.hasPort()) {
        mode = SRTSocketMode::LISTENER;
    }
    else if (!listener.hasPort()) {
        mode = SRTSocketMode::CALLER;
    }
    else {
        mode = SRTSocketMode::RENDEZVOUS;
    }

    // Local interface in caller mode.
    if (local.hasAddress()) {
        if (listener.hasPort()) {
            report.error(u"specify either a listener address or a local outgoing interface for caller mode but not both");
            return false;
        }
        local_address.setAddress(local);
        local_address.clearPort();
    }

    // Listener address is also used in rendezvous mode.
    if (listener.hasPort()) {
        local_address = listener;
    }

    // Caller address, also used in rendezvous mode.
    if (caller.hasAddress()) {
        remote_address = caller;
    }

    return true;
}


//----------------------------------------------------------------------------
// Set options from an srt: URL.
//----------------------------------------------------------------------------

bool ts::SRTArgs::setURL(Report& report, const URL& url)
{
    bool success = true;

    // Check the validity and scheme of the URL.
    if (!url.isValid() || !IsURL(url)) {
        report.error(u"invalid SRT URL: %s", url.toString());
        return false;
    }

    // Definition of parameters in URL query string.
    struct Param {
        // Field to update in SRTArgs.
        bool             SRTArgs::* boolean = nullptr;
        int32_t          SRTArgs::* i32 = nullptr;
        int64_t          SRTArgs::* i64 = nullptr;
        cn::milliseconds SRTArgs::* ms = nullptr;
        UString          SRTArgs::* str = nullptr;
        IPAddress        SRTArgs::* ip = nullptr;
        IPSocketAddress  SRTArgs::* sock = nullptr;
        int              SRTArgs::* enumed = nullptr;

        // Value boundaries or list
        int64_t      min = 0;
        int64_t      max = std::numeric_limits<int64_t>::max();
        const Names* names = nullptr;
    };

    static const Names mode_names = {
        {u"caller",     SRTSocketMode::CALLER},
        {u"listener",   SRTSocketMode::LISTENER},
        {u"rendezvous", SRTSocketMode::RENDEZVOUS},
    };
    static const Names transmission_names = {
        {u"live", SRTTransmissionMode::LIVE},
        {u"file", SRTTransmissionMode::FILE},
    };

    // Define all possible parameters in the query string.
    static const std::map<UString, Param> params = {
        {u"adapter", {.ip = &SRTArgs::_adapter}},
        {u"bind", {.sock = &SRTArgs::_binder}},
        {u"congestion", {.enumed = &SRTArgs::_congestion, .names = &transmission_names}},
        {u"conntimeo", {.ms = &SRTArgs::connection_timeout}},
        {u"cryptomode", {.i32 = &SRTArgs::_cryptomode, .max = 2}},
        /*@@@
        {u"drifttracer", {.boolean = &SRTArgs::}},
        {u"enforcedencryption", {.boolean = &SRTArgs::}},
        {u"fc", {. = &SRTArgs::fc_packets}},    //@@@ bytes or packets ?
        {u"groupconnect", {. = &SRTArgs::_groupconnect, 0, 1}},
        {u"groupminstabletimeo", Param(MILLISECOND}},
        {u"inputbw", {. = &SRTArgs::input_bw}},
        {u"iptos", {. = &SRTArgs::iptos, 0, 255}},
        {u"ipttl", {. = &SRTArgs::ipttl, 1, 255}},
        {u"ipv6only", {. = &SRTArgs::_ipv6only, -1, 1}},
        {u"kmpreannounce", {. = &SRTArgs::kmpreannounce}},
        {u"kmrefreshrate", {. = &SRTArgs::kmrefreshrate}},
        {u"latency", Param(MILLISECOND}},
        {u"linger", {. = &SRTArgs::_linger_time}},
        {u"lossmaxttl", {. = &SRTArgs::lossmaxttl}},
        {u"maxbw", {. = &SRTArgs::max_bw}},
        {u"mininputbw", {. = &SRTArgs::_min_bw}},
        {u"messageapi", {.boolean = &SRTArgs::}},
        {u"minversion", {str = &SRTArgs::}},
        {u"mode", {. = &mode_names}},
        {u"mss", {. = &SRTArgs::mss, 76}},
        {u"nakreport", {.boolean = &SRTArgs::}},
        {u"oheadbw", {. = &SRTArgs::ohead_bw, 5, 100}},
        {u"packetfilter", {str = &SRTArgs::}},
        {u"passphrase", {str = &SRTArgs::}},
        {u"payloadsize", {. = &SRTArgs::payload_size}},
        {u"pbkeylen", {. = &SRTArgs::pbkeylen, 16, 32}},
        {u"peeridletimeo", Param(MILLISECOND}},
        {u"peerlatency", Param(MILLISECOND}},
        {u"port", {. = &SRTArgs::_local_port, 0, 65535}},
        {u"rcvbuf", {. = &SRTArgs::rcvbuf}},
        {u"rcvlatency", Param(MILLISECOND}},
        {u"retransmitalgo", {. = &SRTArgs::_retransmitalgo, 0, 1}},
        {u"sndbuf", {. = &SRTArgs::sndbuf}},
        {u"snddropdelay", Param(MILLISECOND}},
        {u"streamid", {str = &SRTArgs::}},
        {u"tlpktdrop", {.boolean = &SRTArgs::}},
        {u"transtype", {. = &transmission_names}},
        {u"tsbpdmode", {.boolean = &SRTArgs::}},
        @@@@*/
    };

    // Reset synthetic values to check if tye are specified.
    _adapter.clear();
    _binder.clear();
    _congestion = -1;
    _local_port = -1;
    _ipv6only = -1;
    _linger_time = -1;
    _cryptomode = -1;
    _groupconnect = -1;
    _retransmitalgo = -1;
    _min_bw = -1;

    // Analyze all parameters.
    //@@@@

    // Process synthetic values.
    //@@@@

    // Try to guess the mode, local and remove address.
    //@@@@




    report.error(u"SRT URLs are not yet supported"); //@@@
    success = false;




    return success;
}

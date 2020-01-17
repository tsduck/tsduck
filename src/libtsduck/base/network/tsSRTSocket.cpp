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

#include "tsSRTSocket.h"
#include "tsArgs.h"
#include "tsFatal.h"
TSDUCK_SOURCE;

// Currently, we disable SRT on Windows.
#if defined(TS_WINDOWS) && !defined(TS_NOSRT)
#define TS_NOSRT 1
#endif


//----------------------------------------------------------------------------
// Definition of command line arguments.
// These arguments are defined even in the absence of libsrt.
//----------------------------------------------------------------------------

void ts::SRTSocket::defineArgs(ts::Args& args) const
{
    args.option(u"conn-timeout", 0, ts::Args::INTEGER, 0, 1, 0, (1 << 20));
    args.help(u"conn-timeout",
              u"Connect timeout. SRT cannot connect for RTT > 1500 msec (2 handshake exchanges) "
              u"with the default connect timeout of 3 seconds. This option applies to the caller "
              u"and rendezvous connection modes. The connect timeout is 10 times the value set "
              u"for the rendezvous mode (which can be used as a workaround for this connection "
              u"problem with earlier versions.");

    args.option(u"ffs", 0, ts::Args::POSITIVE);
    args.help(u"ffs", u"Flight Flag Size (maximum number of bytes that can be sent without being acknowledged).");

    args.option(u"input-bw", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int64_t>::max());
    args.help(u"input-bw",
              u"This option is effective only if SRTO_MAXBW is set to 0 (relative). It controls "
              u"the maximum bandwidth together with SRTO_OHEADBW option according to the formula: "
              u"MAXBW = INPUTBW * (100 + OHEADBW) / 100. "
              u"When this option is set to 0 (automatic) then the real INPUTBW value will be "
              u"estimated from the rate of the input (cases when the application calls the srt_send* function) "
              u"during transmission."
              u"Recommended: set this option to the predicted bitrate of your live stream and keep default 25% "
              u"value for SRTO_OHEADBW.");

    args.option(u"iptos", 0, ts::Args::INTEGER, 0, 1, 0, 255);
    args.help(u"iptos",
              u"IPv4 Type of Service (see IP_TOS option for IP) or IPv6 Traffic Class "
              u"(see IPV6_TCLASS of IPv6) depending on socket address family. Applies to sender only. "
              u"Sender: user configurable, default: 0xB8.");

    args.option(u"ipttl", 0, ts::Args::INTEGER, 0, 1, 1, 255);
    args.help(u"ipttl",
              u"IPv4 Time To Live (see IP_TTL option for IP) or IPv6 unicast hops "
              u"(see IPV6_UNICAST_HOPS for IPV6) depending on socket address family. "
              u"Applies to sender only, default: 64.");

    args.option(u"enforce-encryption");
    args.help(u"enforce-encryption",
              u"This option enforces that both connection parties have the same passphrase set "
              u"(including empty, that is, with no encryption), or otherwise the connection is rejected.");

    args.option(u"kmrefreshrate", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"kmrefreshrate",
              u"The number of packets to be transmitted after which the Stream Encryption Key (SEK), "
              u"used to encrypt packets, will be switched to the new one. Note that the old and new "
              u"keys live in parallel for a certain period of time (see SRTO_KMPREANNOUNCE) before "
              u"and after the switchover.");

    args.option(u"kmpreannounce", 0, ts::Args::INTEGER, 0, 1, 1, std::numeric_limits<int32_t>::max());
    args.help(u"kmpreannounce",
              u"The interval (defined in packets) between when a new Stream Encrypting Key (SEK) "
              u"is sent and when switchover occurs. This value also applies to the subsequent "
              u"interval between when switchover occurs and when the old SEK is decommissioned. "
              u"Note: The allowed range for this value is between 1 and half of the current value "
              u"of SRTO_KMREFRESHRATE. The minimum value should never be less than the flight "
              u"window (i.e. the number of packets that have already left the sender but have "
              u"not yet arrived at the receiver).");

    args.option(u"latency", 0, ts::Args::POSITIVE);
    args.help(u"latency",
              u"This flag sets both SRTO_RCVLATENCY and SRTO_PEERLATENCY to the same value. "
              u"Note that prior to version 1.3.0 this is the only flag to set the latency, "
              u"however this is effectively equivalent to setting SRTO_PEERLATENCY, when the "
              u"side is sender (see SRTO_SENDER) and SRTO_RCVLATENCY when the side is receiver, "
              u"and the bidirectional stream sending in version 1.2.0is not supported.");

    args.option(u"linger", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"linger", u"Linger time on close, recommended value: 0");

    args.option(u"lossmaxttl", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"lossmaxttl",
              u"The value up to which the Reorder Tolerance may grow. When Reorder Tolerance is > 0, "
              u"then packet loss report is delayed until that number of packets come in. "
              u"Reorder Tolerance increases every time a 'belated' packet has come, but it wasn't due "
              u"to retransmission (that is, when UDP packets tend to come out of order), with the "
              u"difference between the latest sequence and this packet's sequence, and not more "
              u"than the value of this option. By default it's 0, which means that this mechanism "
              u"is turned off, and the loss report is always sent immediately upon "
              u"experiencing a 'gap' in sequences.");

    args.option(u"mss", 0, ts::Args::INTEGER, 0, 1, 76, std::numeric_limits<int32_t>::max());
    args.help(u"mss",
              u"Maximum Segment Size. Used for buffer allocation and rate calculation using "
              u"packet counter assuming fully filled packets. The smallest MSS between the "
              u"peers is used. This is 1500 by default in the overall internet. This is "
              u"the maximum size of the UDP packet and can be only decreased, unless you "
              u"have some unusual dedicated network settings. Not to be mistaken with the "
              u"size of the UDP payload or SRT payload - this size is the size of the IP "
              u"packet, including the UDP and SRT headers.");

    args.option(u"max-bw", 0, ts::Args::INTEGER, 0, 1, -1, std::numeric_limits<int64_t>::max());
    args.help(u"max-bw",
              u"Maximum send bandwidth. NOTE: This option has a default value of -1. "
              u"Although in case when the stream rate is mostly constant it is recommended to "
              u"use value 0 here and shape the bandwidth limit using SRTO_INPUTBW "
              u"and SRTO_OHEADBW options.");

    args.option(u"transtype", 0, ts::Args::STRING);
    args.help(u"transtype",
              u"Sets the transmission type for the socket, in particular, setting this option "
              u"sets multiple other parameters to their default values as required for a "
              u"particular transmission type.");

    args.option(u"messageapi");
    args.help(u"messageapi", u"When set, this socket uses the Message API, otherwise it uses Buffer API.");

    args.option(u"min-version", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"min-version",
              u"The minimum SRT version that is required from the peer. A connection to a peer "
              u"that does not satisfy the minimum version requirement will be rejected.");

    args.option(u"nakreport");
    args.help(u"nakreport",
              u"When this option is specified, the receiver will send UMSG_LOSSREPORT messages periodically "
              u"until the lost packet is retransmitted or intentionally dropped.");

    args.option(u"ohead-bw", 0, ts::Args::INTEGER, 0, 1, 5, 100);
    args.help(u"ohead-bw",
              u"Recovery bandwidth overhead above input rate (see SRTO_INPUTBW). "
              u"It is effective only if SRTO_MAXBW is set to 0.");

    args.option(u"packet-filter", 0, ts::Args::STRING);
    args.help(u"packet-filter",
              u"Set up the packet filter. The string must match appropriate syntax for packet filter setup."
              u"See: https://github.com/Haivision/srt/blob/master/docs/packet-filtering-and-fec.md");

    args.option(u"passphrase", 0, ts::Args::STRING);
    args.help(u"passphrase",
              u"Sets the passphrase for encryption. This turns encryption on on this side (or turns "
              u"it off, if empty passphrase is passed).");

    args.option(u"payload-size", 0, ts::Args::INTEGER, 0, 1, 0, 1456);
    args.help(u"payload-size",
              u"Sets the maximum declared size of a single call to sending function in Live mode. "
              u"Use 0 if this value isn't used (which is default in file mode). This value shall "
              u"not be exceeded for a single data sending instruction in Live mode.");

    args.option(u"pbkeylen", 0, ts::Args::INTEGER, 0, 1, 0, 32);
    args.help(u"pbkeylen",
              u"Sender encryption key length, can be 0, 16 (AES-128), 24 (AES-192), 32 (AES-256).");

    args.option(u"peer-idle-timeout", 0, ts::Args::POSITIVE);
    args.help(u"peer-idle-timeout",
              u"The maximum time in [ms] to wait until any packet is received from peer since "
              u"the last such packet reception. If this time is passed, connection is considered "
              u"broken on timeout.");

    args.option(u"peer-latency", 0, ts::Args::POSITIVE);
    args.help(u"peer-latency",
              u"The latency value (as described in SRTO_RCVLATENCY) that is set by the sender "
              u"side as a minimum value for the receiver.");

    args.option(u"rcvbuf", 0, ts::Args::POSITIVE);
    args.help(u"rcvbuf", u"Receive Buffer Size.");

    args.option(u"rcv-latency", 0, ts::Args::POSITIVE);
    args.help(u"rcv-latency",
              u"The time that should elapse since the moment when the packet was sent and "
              u"the moment when it's delivered to the receiver application in the receiving function.");

    args.option(u"polling-time", 0, ts::Args::POSITIVE);
    args.help(u"polling-time", u"Epoll timeout value (in ms) for non-blocking mode");

    args.option(u"sndbuf", 0, ts::Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"sndbuf", u"Send Buffer Size. Warning: configured in bytes, converted in packets, "
              u"when set, based on MSS value. For desired result, configure MSS first.");

    args.option(u"tlpktdrop", 0, ts::Args::INTEGER, 0, 1, 0, 1);
    args.help(u"tlpktdrop",
              u"Too-late Packet Drop. When enabled on receiver, it skips missing packets that "
              u"have not been delivered in time and delivers the subsequent packets to the "
              u"application when their time-to-play has come. It also sends a fake ACK to the sender. "
              u"When enabled on sender and enabled on the receiving peer, sender drops the older "
              u"packets that have no chance to be delivered in time. It is automatically enabled "
              u"in sender if receiver supports it.");

    args.option(u"streamid", 0, ts::Args::STRING);
    args.help(u"streamid",
              u"A string limited to 512 characters that can be set on the socket prior to connecting. "
              u"This stream ID will be able to be retrieved by the listener side from the socket that "
              u"is returned from srt_accept and was connected by a socket with that set stream ID (so "
              u"you usually use SET on the socket used for srt_connect and GET on the socket retrieved "
              u"from srt_accept). This string can be used completely free-form, however it's highly "
              u"recommended to follow the SRT Access Control guidlines.");

    args.option(u"udp-rcvbuf", 0, ts::Args::POSITIVE);
    args.help(u"udp-rcvbuf", u"UDP Socket Receive Buffer Size.");

    args.option(u"udp-sndbuf", 0, ts::Args::POSITIVE);
    args.help(u"udp-sndbuf", u"UDP Socket Send Buffer Size.");
}


//----------------------------------------------------------------------------
// Stubs in the absence of libsrt.
//----------------------------------------------------------------------------

#if defined(TS_NOSRT)

#define NOSRT_ERROR_MSG u"This version of TSDuck was compiled without SRT support"
#define NOSRT_ERROR { report.error(NOSRT_ERROR_MSG); return false; }

ts::SRTSocket::SRTSocket(SRTSocketMode mode, Report& report) : _guts(nullptr) {}
ts::SRTSocket::~SRTSocket() {}
bool ts::SRTSocket::open(const ts::SocketAddress& addr, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::close(ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::loadArgs(ts::DuckContext& duck, ts::Args& args) { return true; }
bool ts::SRTSocket::send(const void* data, size_t size, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, ts::Report& report) const NOSRT_ERROR
int  ts::SRTSocket::getSocket() const { return -1; }
bool ts::SRTSocket::getMessageApi() const { return false; }
ts::UString ts::SRTSocket::GetLibraryVersion() { return NOSRT_ERROR_MSG; }

#else


//----------------------------------------------------------------------------
// Actual libsrt implementation.
//----------------------------------------------------------------------------

#define DEFAULT_POLLING_TIME 100

// The srtlib header contains errors.
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(documentation)
TS_LLVM_NOWARNING(old-style-cast)
#include <srt/srt.h>
TS_POP_WARNING()


//----------------------------------------------------------------------------
// Get the version of the SRT library.
//----------------------------------------------------------------------------

ts::UString ts::SRTSocket::GetLibraryVersion()
{
    return UString::Format(u"libsrt version %d.%d.%d", {SRT_VERSION_MAJOR, SRT_VERSION_MINOR, SRT_VERSION_PATCH});
}


//----------------------------------------------------------------------------
// Internal representation ("guts")
//----------------------------------------------------------------------------

class ts::SRTSocket::Guts
{
     TS_NOBUILD_NOCOPY(Guts);
public:
     // Default constructor.
     explicit Guts(SRTSocketMode mode);

     bool send(const void* data, size_t size, const SocketAddress& dest, Report& report);
     bool setDefaultAddress(const UString& name, Report& report);
     bool setDefaultAddress(const SocketAddress& addr, Report& report);
     bool setSockOpt(int optName, const char* optNameStr, const void* optval, int optlen, Report& report);
     bool setSockOptPre(Report& report);
     bool setSockOptPost(Report& report);
     int srtListen(const SocketAddress& addr, Report& report);
     int srtConnect(const SocketAddress& addr, Report& report);

     // Socket working data.
     SocketAddress _default_address;
     SRTSocketMode _mode;
     int           _sock;

     // Socket options.
     SRT_TRANSTYPE _transtype;
     std::string   _packet_filter;
     std::string   _passphrase;
     std::string   _streamid;
     int     _polling_time;
     bool    _messageapi;
     bool    _nakreport;
     int     _conn_timeout;
     int     _ffs;
     int     _linger;
     int     _lossmaxttl;
     int     _mss;
     int     _ohead_bw;
     int     _payload_size;
     int     _rcvbuf;
     int     _sndbuf;
     bool    _enforce_encryption;
     int32_t _kmrefreshrate;
     int32_t _kmpreannounce;
     int     _udp_rcvbuf;
     int     _udp_sndbuf;
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
     bool    _tlpktdrop;
};


//----------------------------------------------------------------------------
// Guts constructor.
//----------------------------------------------------------------------------

ts::SRTSocket::Guts::Guts(SRTSocketMode mode) :
    _default_address(),
    _mode(mode),
    _sock(TS_SOCKET_T_INVALID),
    _transtype(SRTT_INVALID),
    _packet_filter(),
    _passphrase(),
    _streamid(),
    _polling_time(-1),
    _messageapi(false),
    _nakreport(false),
    _conn_timeout(-1),
    _ffs(-1),
    _linger(-1),
    _lossmaxttl(-1),
    _mss(-1),
    _ohead_bw(-1),
    _payload_size(-1),
    _rcvbuf(-1),
    _sndbuf(-1),
    _enforce_encryption(false),
    _kmrefreshrate(-1),
    _kmpreannounce(-1),
    _udp_rcvbuf(-1),
    _udp_sndbuf(-1),
    _input_bw(-1),
    _max_bw(-1),
    _iptos(-1),
    _ipttl(-1),
    _latency(-1),
    _min_version(-1),
    _pbkeylen(-1),
    _peer_idle_timeout(-1),
    _peer_latency(-1),
    _rcv_latency(-1),
    _tlpktdrop(false)
{
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SRTSocket::SRTSocket(SRTSocketMode mode, Report& report) :
    _guts(new Guts(mode))
{
    CheckNonNull(_guts);
    srt_startup();
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::SRTSocket::~SRTSocket(void)
{
    if (_guts != nullptr) {
        close();
        srt_cleanup();
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Basic getters (from guts).
//----------------------------------------------------------------------------

int ts::SRTSocket::getSocket() const
{
    return _guts->_sock;
}

bool ts::SRTSocket::getMessageApi() const
{
    return _guts->_messageapi;
}


//----------------------------------------------------------------------------
// Set a default destination address and port for outgoing messages.
// Both address and port are mandatory in socket address.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::setDefaultAddress(const ts::UString& name, ts::Report& report)
{
    ts::SocketAddress addr;
    return addr.resolve(name, report) && setDefaultAddress(addr, report);
}

bool ts::SRTSocket::Guts::setDefaultAddress(const ts::SocketAddress& addr, ts::Report& report)
{
    if (!addr.hasAddress()) {
        report.error(u"missing IP address in UDP destination");
        return false;
    }
    else if (!addr.hasPort()) {
        report.error(u"missing port number in UDP destination");
        return false;
    }
    else {
        _default_address = addr;
        return true;
    }
}


//----------------------------------------------------------------------------
// Open the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SRTSocket::open(const ts::SocketAddress& addr, ts::Report& report)
{
    int ret = 0;
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    // Only supports IPv4.
    _guts->_sock = srt_socket(AF_INET, SOCK_DGRAM, 0);
    if (_guts->_sock < 0) {
        report.error(u"error during srt_socket(), msg: %s", { srt_getlasterror_str() });
        return false;
    }

    if (!_guts->setSockOptPre(report)) {
        goto fail;
    }

    switch (_guts->_mode) {
        case LISTENER:
            ret = _guts->srtListen(addr, report);
            if (ret < 0) {
                goto fail;
            }
            _guts->_sock = ret;
            break;
        case CALLER:
            ret = _guts->srtConnect(addr, report);
            if (ret < 0) {
                goto fail;
            }
            break;
        case RENDEZVOUS:
            // TODO not supported for now.
        case LEN:
        default:
            report.error(u"unsupported socket mode");
            goto fail;
    }

    if (!_guts->setSockOptPost(report)) {
        goto fail;
    }

    return true;

fail:
    close(report);
    return false;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::close(ts::Report& report)
{
    if (_guts->_sock >= 0) {
        srt_close(_guts->_sock);
        _guts->_sock = TS_SOCKET_T_INVALID;
    }
    return true;
}


//----------------------------------------------------------------------------
// Load command line arguments.
//----------------------------------------------------------------------------

bool ts::SRTSocket::loadArgs(ts::DuckContext& duck, ts::Args& args)
{
    const UString ttype(args.value(u"transtype", u"live"));
    if (ttype != u"live" && ttype != u"file") {
        return false;
    }

    _guts->_transtype = (ttype == u"live") ? SRTT_LIVE : SRTT_FILE;
    _guts->_nakreport = args.present(u"nakreport");
    _guts->_conn_timeout = args.intValue<int>(u"conn-timeout", -1);
    _guts->_messageapi = args.present(u"messageapi");
    _guts->_ffs = args.intValue<int>(u"ffs", -1);
    _guts->_input_bw = args.intValue<int64_t>(u"input-bw", -1);
    _guts->_iptos = args.intValue<int32_t>(u"iptos", -1);
    _guts->_ipttl = args.intValue<int32_t>(u"ipttl", -1);
    _guts->_enforce_encryption = args.present(u"enforce-encryption");
    _guts->_kmrefreshrate = args.intValue<int32_t>(u"kmrefreshrate", -1);
    _guts->_kmpreannounce = args.intValue<int32_t>(u"kmpreannounce", -1);
    _guts->_latency = args.intValue<int32_t>(u"latency", -1);
    _guts->_linger = args.intValue<int>(u"linger", -1);
    _guts->_lossmaxttl = args.intValue<int>(u"lossmaxttl", -1);
    _guts->_max_bw = args.intValue<int64_t>(u"max-bw", -1);
    _guts->_min_version = args.intValue<int32_t>(u"min-version", -1);
    _guts->_mss = args.intValue<int>(u"mss", -1);
    _guts->_ohead_bw = args.intValue<int>(u"ohead-bw", -1);
    _guts->_streamid = args.value(u"streamid").toUTF8();
    _guts->_packet_filter = args.value(u"packet-filter").toUTF8();
    _guts->_passphrase = args.value(u"passphrase").toUTF8();
    _guts->_payload_size = args.intValue<int>(u"payload-size", -1);
    _guts->_pbkeylen = args.intValue<int32_t>(u"pbkeylen", -1);
    _guts->_peer_idle_timeout = args.intValue<int32_t>(u"peer-idle-timeout", -1);
    _guts->_peer_latency = args.intValue<int32_t>(u"peer-latency", -1);
    _guts->_rcvbuf = args.intValue<int>(u"rcvbuf", -1);
    _guts->_rcv_latency = args.intValue<int32_t>(u"rcv-latency", -1);
    _guts->_polling_time = args.intValue<int>(u"polling-time", DEFAULT_POLLING_TIME);
    _guts->_sndbuf = args.intValue<int>(u"sndbuf", -1);
    _guts->_tlpktdrop = args.present(u"tlpktdrop");
    _guts->_udp_rcvbuf = args.intValue<int>(u"udp-rcvbuf", -1);
    _guts->_udp_sndbuf = args.intValue<int>(u"udp-sndbuf", -1);

    return true;
}


//----------------------------------------------------------------------------
// Set/get socket option.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::setSockOpt(int optName, const char* optNameStr, const void* optval, int optlen, ts::Report& report)
{
    int ret = srt_setsockflag(_sock, SRT_SOCKOPT(optName), optval, optlen);
    if (ret < 0) {
        report.error(u"error during srt_setsockflag(%s), msg: %s", { optNameStr, srt_getlasterror_str() });
        return false;
    }
    return true;
}

bool ts::SRTSocket::getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, ts::Report& report) const
{
    int ret = srt_getsockflag(_guts->_sock, SRT_SOCKOPT(optName), optval, &optlen);
    if (ret < 0) {
        report.error(u"error during srt_getsockflag(%s), msg: %s", { optNameStr, srt_getlasterror_str() });
        return false;
    }
    return true;
}

bool ts::SRTSocket::Guts::setSockOptPre(ts::Report& report)
{
    const int yes     = 1;
    const int msgapi  = _messageapi ? 1 : 0;
    const int encrypt = _enforce_encryption ? 1 : 0;
    const int nakrep  = _nakreport ? 1 : 0;
    const int pktdrop = _tlpktdrop ? 1 : 0;

    if ((_mode != SRTSocketMode::CALLER && !setSockOpt(SRTO_SENDER, "SRTO_SENDER", &yes, sizeof(yes), report)) ||
        (_transtype != SRTT_INVALID && !setSockOpt(SRTO_TRANSTYPE, "SRTO_TRANSTYPE", &_transtype, sizeof(_transtype), report)) ||
        (!setSockOpt(SRTO_MESSAGEAPI, "SRTO_MESSAGEAPI", &msgapi, sizeof(msgapi), report)) ||
        (_conn_timeout >= 0 && !setSockOpt(SRTO_CONNTIMEO, "SRTO_CONNTIMEO", &_conn_timeout, sizeof(_conn_timeout), report)) ||
        (_ffs > 0 && !setSockOpt(SRTO_FC, "SRTO_FC", &_ffs, sizeof(_ffs), report)) ||
        (_iptos >= 0 && !setSockOpt(SRTO_IPTOS, "SRTO_IPTOS", &_iptos, sizeof(_iptos), report)) ||
        (_ipttl > 0 && !setSockOpt(SRTO_IPTTL, "SRTO_IPTTL", &_ipttl, sizeof(_ipttl), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (_enforce_encryption && !setSockOpt(SRTO_ENFORCEDENCRYPTION, "SRTO_ENFORCEDENCRYPTION", &encrypt, sizeof(encrypt), report)) ||
#endif
        (_kmrefreshrate >= 0 && !setSockOpt(SRTO_KMREFRESHRATE, "SRTO_KMREFRESHRATE", &_kmrefreshrate, sizeof(_kmrefreshrate), report)) ||
        (_kmpreannounce > 0 && !setSockOpt(SRTO_KMPREANNOUNCE, "SRTO_KMPREANNOUNCE", &_kmpreannounce, sizeof(_kmpreannounce), report)) ||
        (_latency > 0 && !setSockOpt(SRTO_LATENCY, "SRTO_LATENCY", &_latency, sizeof(_latency), report)) ||
        (_linger >= 0 && !setSockOpt(SRTO_LINGER, "SRTO_LINGER", &_linger, sizeof(_linger), report)) ||
        (_lossmaxttl >= 0 && !setSockOpt(SRTO_LOSSMAXTTL, "SRTO_LOSSMAXTTL", &_lossmaxttl, sizeof(_lossmaxttl), report)) ||
        (_max_bw >= 0 && !setSockOpt(SRTO_MAXBW, "SRTO_MAXBW", &_max_bw, sizeof(_max_bw), report)) ||
        (_min_version > 0 && !setSockOpt(SRTO_MINVERSION, "SRTO_MINVERSION", &_min_version, sizeof(_min_version), report)) ||
        (_mss >= 0 && !setSockOpt(SRTO_MSS, "SRTO_MSS", &_mss, sizeof(_mss), report)) ||
        (_nakreport && !setSockOpt(SRTO_NAKREPORT, "SRTO_NAKREPORT", &nakrep, sizeof(nakrep), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (!_packet_filter.empty() && !setSockOpt(SRTO_PACKETFILTER, "SRTO_PACKETFILTER", _packet_filter.c_str(), int(_packet_filter.size()), report)) ||
#endif
        (!_passphrase.empty() && !setSockOpt(SRTO_PASSPHRASE, "SRTO_PASSPHRASE", _passphrase.c_str(), int(_passphrase.size()), report)) ||
        (!_streamid.empty() && !setSockOpt(SRTO_STREAMID, "SRTO_STREAMID", _streamid.c_str(), int(_streamid.size()), report)) ||
        (_payload_size > 0 && !setSockOpt(SRTO_PAYLOADSIZE, "SRTO_PAYLOADSIZE", &_payload_size, sizeof(_payload_size), report)) ||
        (_pbkeylen > 0 && !setSockOpt(SRTO_PBKEYLEN, "SRTO_PBKEYLEN", &_pbkeylen, sizeof(_pbkeylen), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (_peer_idle_timeout > 0 && !setSockOpt(SRTO_PEERIDLETIMEO, "SRTO_PEERIDLETIMEO", &_peer_idle_timeout, sizeof(_peer_idle_timeout), report)) ||
#endif
        (_peer_latency > 0 && !setSockOpt(SRTO_PEERLATENCY, "SRTO_PEERLATENCY", &_peer_latency, sizeof(_peer_latency), report)) ||
        (_rcvbuf > 0 && !setSockOpt(SRTO_RCVBUF, "SRTO_RCVBUF", &_rcvbuf, sizeof(_rcvbuf), report)) ||
        (_rcv_latency > 0 && !setSockOpt(SRTO_RCVLATENCY, "SRTO_RCVLATENCY", &_rcv_latency, sizeof(_rcv_latency), report)) ||
        (_sndbuf > 0 && !setSockOpt(SRTO_SNDBUF, "SRTO_SNDBUF", &_sndbuf, sizeof(_sndbuf), report)) ||
        (_tlpktdrop && !setSockOpt(SRTO_TLPKTDROP, "SRTO_TLPKTDROP", &pktdrop, sizeof(pktdrop), report)))
    {
        return false;
    }

    // In case of error here, use system default
    if (_udp_rcvbuf > 0) {
        setSockOpt(SRTO_UDP_RCVBUF, "SRTO_UDP_RCVBUF", &_udp_rcvbuf, sizeof(_udp_rcvbuf), report);
    }
    if (_udp_sndbuf > 0) {
        setSockOpt(SRTO_UDP_SNDBUF, "SRTO_UDP_SNDBUF", &_udp_sndbuf, sizeof(_udp_sndbuf), report);
    }
    return true;
}

bool ts::SRTSocket::Guts::setSockOptPost(ts::Report& report)
{
    if (_max_bw == 0 && (
        (_input_bw >= 0 && !setSockOpt(SRTO_INPUTBW, "SRTO_INPUTBW", &_input_bw, sizeof(_input_bw), report)) ||
        (_ohead_bw >= 5 && !setSockOpt(SRTO_OHEADBW, "SRTO_OHEADBW", &_ohead_bw, sizeof(_ohead_bw), report))))
    {
        return false;
    }

    return true;
}

int ts::SRTSocket::Guts::srtListen(const ts::SocketAddress& addr, ts::Report& report)
{
    int ret, reuse = 1, peer_addr_len;
    ::sockaddr sock_addr, peer_addr;
    addr.copy(sock_addr);

    ret = setSockOpt(SRTO_REUSEADDR, "SRTO_REUSEADDR", &reuse, sizeof(reuse), report);
    if (ret < 0) {
        // Error handling if needed
    }

    ret = srt_bind(_sock, &sock_addr, sizeof(sock_addr));
    if (ret) {
        report.error(u"error during srt_bind(), msg: %s", { srt_getlasterror_str() });
        return -1;
    }

    // Second parameter is the number of simultaneous connection accepted. For now we only accept one.
    ret = srt_listen(_sock, 1);
    if (ret) {
        report.error(u"error during srt_listen(), msg: %s", { srt_getlasterror_str() });
        return -1;
    }

    ret = srt_accept(_sock, &peer_addr, &peer_addr_len);
    if (ret < 0) {
        report.error(u"error during srt_accept(), msg: %s", { srt_getlasterror_str() });
        return -1;
    }

    SocketAddress p_addr(peer_addr);
    if (!setDefaultAddress(p_addr, report)) {
        srt_close(ret);
        return -1;
    }

    return ret;
}

int ts::SRTSocket::Guts::srtConnect(const ts::SocketAddress& addr, ts::Report& report)
{
    int ret;
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    ret = srt_connect(_sock, &sock_addr, sizeof(sock_addr));
    if (ret < 0) {
        report.error(u"error during srt_connect, msg: %s", { srt_getlasterror_str() });
        return ret;
    }

    return ret;
}

//----------------------------------------------------------------------------
// Send a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::SRTSocket::send(const void* data, size_t size, ts::Report& report)
{
    return _guts->send(data, size, _guts->_default_address, report);
}

bool ts::SRTSocket::Guts::send(const void* data, size_t size, const ts::SocketAddress& dest, ts::Report& report)
{
    int ret = srt_send(_sock, reinterpret_cast<const char*>(data), int(size));
    if (srt_getlasterror(nullptr) == SRT_EASYNCSND) {
        report.warning(u"not enough space to send data with size %d, msg: %s", { size, srt_getlasterror_str() });
        return true;
    }
    if (ret < 0) {
        report.error(u"error during srt_send(), msg: %s", { srt_getlasterror_str() });
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Receive a message.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, ts::Report& report)
{
    const int ret = srt_recv(_guts->_sock, reinterpret_cast<char*>(data), int(max_size));
    // For non-blocking mode only, if no data available yet
    if (srt_getlasterror(nullptr) == SRT_EASYNCRCV) {
        ret_size = 0;
        return true;
    }
    else if (ret < 0) {
        report.error(u"error during srt_recv(), msg: %s", { srt_getlasterror_str() });
        return false;
    }
    else {
        ret_size = size_t(ret);
        return true;
    }
}

#endif // TS_NOSRT

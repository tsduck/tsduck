//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Anthony Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSRTSocket.h"
#include "tsSingleton.h"
#include "tsArgs.h"
#include "tsjsonObject.h"
#include "tsTime.h"
#include "tsMemory.h"
#include "tsNullReport.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Definition of command line arguments.
// These arguments are defined even in the absence of libsrt.
//----------------------------------------------------------------------------

void ts::SRTSocket::defineArgs(ts::Args& args)
{
    args.option(u"caller", 'c', Args::STRING);
    args.help(u"caller", u"address:port",
              u"Use SRT in caller (or rendezvous) mode. "
              u"The parameter specifies the IPv4 remote address (or host name) and UDP port. "
              u"If --listener is also specified, the SRT socket works in rendezvous mode.");

    args.option(u"listener", 'l', Args::STRING);
    args.help(u"listener", u"[address:]port",
              u"Use SRT in listener (or rendezvous) mode. "
              u"The parameter specifies the IPv4 local address and UDP port on which the SRT socket listens. "
              u"The address is optional, the port is mandatory. "
              u"If --caller is also specified, the SRT socket works in rendezvous mode.");

    args.option(u"backlog", 0, Args::POSITIVE);
    args.help(u"backlog",
              u"With --listener, specify the number of allowed waiting incoming clients. "
              u"The default is one.");

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"With --listener, disable the reuse port socket option. "
              u"Do not use unless completely necessary.");

    args.option(u"local-interface", 0, Args::STRING);
    args.help(u"local-interface", u"address",
              u"In caller mode, use the specified local IP interface for outgoing connections. "
              u"This option is incompatible with --listener.");

    args.option(u"conn-timeout", 0, Args::INTEGER, 0, 1, 0, (1 << 20));
    args.help(u"conn-timeout",
              u"Connect timeout. SRT cannot connect for RTT > 1500 msec (2 handshake exchanges) "
              u"with the default connect timeout of 3 seconds. This option applies to the caller "
              u"and rendezvous connection modes. The connect timeout is 10 times the value set "
              u"for the rendezvous mode (which can be used as a workaround for this connection "
              u"problem with earlier versions).");

    args.option(u"ffs", 0, Args::POSITIVE);
    args.help(u"ffs",
              u"Flight Flag Size (maximum number of bytes that can be sent without being acknowledged).");

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

    args.option(u"enforce-encryption");
    args.help(u"enforce-encryption",
              u"This option enforces that both connection parties have the same passphrase set "
              u"(including empty, that is, with no encryption), or otherwise the connection is rejected.");

    args.option(u"kmrefreshrate", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"kmrefreshrate",
              u"The number of packets to be transmitted after which the Stream Encryption Key (SEK), "
              u"used to encrypt packets, will be switched to the new one. Note that the old and new "
              u"keys live in parallel for a certain period of time (see SRTO_KMPREANNOUNCE) before "
              u"and after the switchover.");

    args.option(u"kmpreannounce", 0, Args::INTEGER, 0, 1, 1, std::numeric_limits<int32_t>::max());
    args.help(u"kmpreannounce",
              u"The interval (defined in packets) between when a new Stream Encrypting Key (SEK) "
              u"is sent and when switchover occurs. This value also applies to the subsequent "
              u"interval between when switchover occurs and when the old SEK is decommissioned. "
              u"Note: The allowed range for this value is between 1 and half of the current value "
              u"of SRTO_KMREFRESHRATE. The minimum value should never be less than the flight "
              u"window (i.e. the number of packets that have already left the sender but have "
              u"not yet arrived at the receiver).");

    args.option(u"latency", 0, Args::POSITIVE);
    args.help(u"latency",
              u"This flag sets both SRTO_RCVLATENCY and SRTO_PEERLATENCY to the same value. "
              u"Note that prior to version 1.3.0 this is the only flag to set the latency, "
              u"however this is effectively equivalent to setting SRTO_PEERLATENCY, when the "
              u"side is sender (see SRTO_SENDER) and SRTO_RCVLATENCY when the side is receiver, "
              u"and the bidirectional stream sending in version 1.2.0is not supported.");

    args.option(u"linger", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"linger",
              u"Linger time on close. Define how long, in seconds, to enable queued "
              u"data to be sent after end of stream. Default: no linger.");

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

    args.option(u"mss", 0, Args::INTEGER, 0, 1, 76, std::numeric_limits<int32_t>::max());
    args.help(u"mss",
              u"Maximum Segment Size. Used for buffer allocation and rate calculation using "
              u"packet counter assuming fully filled packets. The smallest MSS between the "
              u"peers is used. This is 1500 by default in the overall internet. This is "
              u"the maximum size of the UDP packet and can be only decreased, unless you "
              u"have some unusual dedicated network settings. Not to be mistaken with the "
              u"size of the UDP payload or SRT payload - this size is the size of the IP "
              u"packet, including the UDP and SRT headers.");

    args.option(u"max-bw", 0, Args::INTEGER, 0, 1, -1, std::numeric_limits<int64_t>::max());
    args.help(u"max-bw",
              u"Maximum send bandwidth. NOTE: This option has a default value of -1. "
              u"Although in case when the stream rate is mostly constant it is recommended to "
              u"use value 0 here and shape the bandwidth limit using SRTO_INPUTBW "
              u"and SRTO_OHEADBW options.");

    args.option(u"transtype", 0, Args::STRING);
    args.help(u"transtype",
              u"Sets the transmission type for the socket, in particular, setting this option "
              u"sets multiple other parameters to their default values as required for a "
              u"particular transmission type.");

    args.option(u"bufferapi");
    args.help(u"bufferapi", u"When set, this socket uses the Buffer API. The default is Message API.");

    args.option(u"messageapi");
    args.help(u"messageapi", u"Use the Message API. This is now the default, use --bufferapi to disable it.");

    args.option(u"min-version", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"min-version",
              u"The minimum SRT version that is required from the peer. A connection to a peer "
              u"that does not satisfy the minimum version requirement will be rejected.");

    args.option(u"nakreport");
    args.help(u"nakreport",
              u"When this option is specified, the receiver will send UMSG_LOSSREPORT messages periodically "
              u"until the lost packet is retransmitted or intentionally dropped.");

    args.option(u"ohead-bw", 0, Args::INTEGER, 0, 1, 5, 100);
    args.help(u"ohead-bw",
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
    args.help(u"pbkeylen",
              u"Sender encryption key length, can be 0, 16 (AES-128), 24 (AES-192), 32 (AES-256).");

    args.option(u"peer-idle-timeout", 0, Args::POSITIVE);
    args.help(u"peer-idle-timeout",
              u"The maximum time in [ms] to wait until any packet is received from peer since "
              u"the last such packet reception. If this time is passed, connection is considered "
              u"broken on timeout.");

    args.option(u"peer-latency", 0, Args::POSITIVE);
    args.help(u"peer-latency",
              u"The latency value (as described in SRTO_RCVLATENCY) that is set by the sender "
              u"side as a minimum value for the receiver.");

    args.option(u"rcvbuf", 0, Args::POSITIVE);
    args.help(u"rcvbuf", u"Receive Buffer Size.");

    args.option(u"rcv-latency", 0, Args::POSITIVE);
    args.help(u"rcv-latency",
              u"The time that should elapse since the moment when the packet was sent and "
              u"the moment when it's delivered to the receiver application in the receiving function.");

    args.option(u"polling-time", 0, Args::POSITIVE);
    args.help(u"polling-time", u"Epoll timeout value (in ms) for non-blocking mode");

    args.option(u"sndbuf", 0, Args::INTEGER, 0, 1, 0, std::numeric_limits<int32_t>::max());
    args.help(u"sndbuf", u"Send Buffer Size. Warning: configured in bytes, converted in packets, "
              u"when set, based on MSS value. For desired result, configure MSS first.");

    args.option(u"tlpktdrop", 0, Args::INTEGER, 0, 1, 0, 1);
    args.help(u"tlpktdrop",
              u"Too-late Packet Drop. When enabled on receiver, it skips missing packets that "
              u"have not been delivered in time and delivers the subsequent packets to the "
              u"application when their time-to-play has come. It also sends a fake ACK to the sender. "
              u"When enabled on sender and enabled on the receiving peer, sender drops the older "
              u"packets that have no chance to be delivered in time. It is automatically enabled "
              u"in sender if receiver supports it.");

    args.option(u"statistics-interval", 0, Args::POSITIVE);
    args.help(u"statistics-interval", u"milliseconds",
              u"Report SRT usage statistics at regular intervals, in milliseconds. "
              u"The specified interval is a minimum value, actual reporting can occur "
              u"only when data are exchanged over the SRT socket.");

    args.option(u"final-statistics");
    args.help(u"final-statistics",
              u"Report SRT usage statistics when the SRT socket is closed. "
              u"This option is implicit with --statistics-interval.");

    args.option(u"json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"json-line", u"'prefix'",
              u"With --statistics-interval or --final-statistics, report the statistics as one single line in JSON format. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");

    args.option(u"streamid", 0, Args::STRING);
    args.help(u"streamid",
              u"A string limited to 512 characters that can be set on the socket prior to connecting. "
              u"This stream ID will be able to be retrieved by the listener side from the socket that "
              u"is returned from srt_accept and was connected by a socket with that set stream ID (so "
              u"you usually use SET on the socket used for srt_connect and GET on the socket retrieved "
              u"from srt_accept). This string can be used completely free-form, however it's highly "
              u"recommended to follow the SRT Access Control guidlines.");

    args.option(u"udp-rcvbuf", 0, Args::POSITIVE);
    args.help(u"udp-rcvbuf", u"UDP socket receive buffer size in bytes.");

    args.option(u"udp-sndbuf", 0, Args::POSITIVE);
    args.help(u"udp-sndbuf", u"UDP socket send buffer size in bytes.");
}


//----------------------------------------------------------------------------
// Stubs in the absence of libsrt.
//----------------------------------------------------------------------------

#if defined(TS_NO_SRT)

#define NOSRT_ERROR_MSG u"This version of TSDuck was compiled without SRT support"
#define NOSRT_ERROR { report.error(NOSRT_ERROR_MSG); return false; }

ts::SRTSocket::SRTSocket() : _guts(nullptr) {}
ts::SRTSocket::~SRTSocket() {}
bool ts::SRTSocket::open(SRTSocketMode, const IPv4SocketAddress&, const IPv4SocketAddress&, Report& report) NOSRT_ERROR
bool ts::SRTSocket::close(Report& report) NOSRT_ERROR
bool ts::SRTSocket::peerDisconnected() const { return false; }
bool ts::SRTSocket::loadArgs(DuckContext&, Args&) { return true; }
bool ts::SRTSocket::send(const void*, size_t, Report& report) NOSRT_ERROR
bool ts::SRTSocket::receive(void*, size_t, size_t&, Report& report) NOSRT_ERROR
bool ts::SRTSocket::receive(void*, size_t, size_t&, MicroSecond&, Report& report) NOSRT_ERROR
bool ts::SRTSocket::reportStatistics(SRTStatMode, Report& report) NOSRT_ERROR
bool ts::SRTSocket::getSockOpt(int, const char*, void*, int&, Report& report) const NOSRT_ERROR
int  ts::SRTSocket::getSocket() const { return -1; }
bool ts::SRTSocket::getMessageApi() const { return false; }
ts::UString ts::SRTSocket::GetLibraryVersion() { return NOSRT_ERROR_MSG; }
bool ts::SRTSocket::setAddressesInternal(const UString&, const UString&, const UString&, bool, Report& report) NOSRT_ERROR
size_t ts::SRTSocket::totalSentBytes() const { return 0; }
size_t ts::SRTSocket::totalReceivedBytes() const { return 0; }

#else


//----------------------------------------------------------------------------
// Actual libsrt implementation.
//----------------------------------------------------------------------------

#define DEFAULT_POLLING_TIME 100

// The srtlib headers contain errors.
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(documentation)
TS_LLVM_NOWARNING(old-style-cast)
TS_LLVM_NOWARNING(undef)
TS_GCC_NOWARNING(undef)
TS_GCC_NOWARNING(effc++)
TS_MSC_NOWARNING(4005)  // 'xxx' : macro redefinition
TS_MSC_NOWARNING(4668)  // 'xxx' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'

// Bug in GCC: "#if __APPLE__" triggers -Werror=undef despite TS_GCC_NOWARNING(undef)
// This is a known GCC bug since 2012, never fixed: #if is too early in lex analysis and #pragma are not yet parsed.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
#if defined(TS_GCC_ONLY) && !defined(__APPLE__)
    #define __APPLE__ 0
    #define ZERO__APPLE__ 1
#endif

#include <srt/srt.h>

// The header access_control.h was introduced in version 1.4.2.
// On Windows, access_control.h was missing in the binary installer before 1.5.3.
#if SRT_VERSION_VALUE < SRT_MAKE_VERSION_VALUE(1,4,2)
    typedef SRT_REJECT_REASON RejectReason;
#else
    #define HAS_SRT_ACCESS_CONTROL 1
    typedef int RejectReason;
    #if defined(TS_WINDOWS) && SRT_VERSION_VALUE < SRT_MAKE_VERSION_VALUE(1,5,3)
        #define SRT_REJX_OVERLOAD 1402 // manually defined when header is missing.
    #else
        #include <srt/access_control.h>
    #endif
#endif

#if defined(ZERO__APPLE__)
    #undef __APPLE__
    #undef ZERO__APPLE__
#endif

TS_POP_WARNING()


//----------------------------------------------------------------------------
// A global singleton which initializes SRT.
// The SRT library is initialized when the first SRT socket is opened
// and terminated when the last socket is closed.
//----------------------------------------------------------------------------

namespace {
    class SRTInit
    {
        TS_DECLARE_SINGLETON(SRTInit);
    public:
        ~SRTInit();
    };

    TS_DEFINE_SINGLETON(SRTInit);

    // Singleton constructor, initialize SRT once.
    SRTInit::SRTInit()
    {
        ::srt_startup();
    }

    // Singleton destructor, cleanup SRT on application exit.
    SRTInit::~SRTInit()
    {
        ::srt_cleanup();
    }
}


//----------------------------------------------------------------------------
// Get the version of the SRT library.
//----------------------------------------------------------------------------

ts::UString ts::SRTSocket::GetLibraryVersion()
{
#if defined(SRT_VERSION_STRING)
    return UString::Format(u"libsrt version %s", {SRT_VERSION_STRING});
#else
    return UString::Format(u"libsrt version %d.%d.%d", {SRT_VERSION_MAJOR, SRT_VERSION_MINOR, SRT_VERSION_PATCH});
#endif
}


//----------------------------------------------------------------------------
// Internal representation ("guts")
//----------------------------------------------------------------------------

class ts::SRTSocket::Guts
{
     TS_NOBUILD_NOCOPY(Guts);
private:
     SRTSocket* _parent;
public:
     // Default constructor.
     Guts(SRTSocket* parent) : _parent(parent) {}

     bool send(const void* data, size_t size, const IPv4SocketAddress& dest, Report& report);
     bool setSockOpt(int optName, const char* optNameStr, const void* optval, size_t optlen, Report& report);
     bool setSockOptPre(Report& report);
     bool setSockOptPost(Report& report);
     bool srtListen(const IPv4SocketAddress& addr, Report& report);
     bool srtConnect(const IPv4SocketAddress& addr, Report& report);
     bool srtBind(const IPv4SocketAddress& addr, Report& report);
     bool reportStats(Report& report);

     // Socket working data.
     IPv4SocketAddress    local_address {};
     IPv4SocketAddress    remote_address {};
     SRTSocketMode        mode = SRTSocketMode::DEFAULT;
     volatile ::SRTSOCKET sock = SRT_INVALID_SOCK;       // SRT socket for data transmission
     volatile ::SRTSOCKET listener  = SRT_INVALID_SOCK;   // Listener SRT socket when srt_listen() is used.
     size_t               total_sent_bytes = 0;
     size_t               total_received_bytes = 0;
     Time                 next_stats {};

     // Socket options.
     ::SRT_TRANSTYPE transtype = SRTT_INVALID;
     std::string packet_filter {};
     std::string passphrase {};
     std::string streamid {};
     int         polling_time = -1;
     bool        messageapi = false;
     bool        nakreport = false;
     bool        reuse_port = false;
     int         backlog = 0;
     int         conn_timeout = -1;
     int         ffs = -1;
     ::linger    linger_opt {0, 0};
     int         lossmaxttl = -1;
     int         mss = -1;
     int         ohead_bw = -1;
     int         payload_size = -1;
     int         rcvbuf = -1;
     int         sndbuf = -1;
     bool        enforce_encryption = false;
     int32_t     kmrefreshrate = -1;
     int32_t     kmpreannounce = -1;
     int         udp_rcvbuf = -1;
     int         udp_sndbuf = -1;
     int64_t     input_bw = -1;
     int64_t     max_bw = -1;
     int32_t     iptos = -1;
     int32_t     ipttl = -1;
     int32_t     latency = -1;
     int32_t     min_version = -1;
     int32_t     pbkeylen = -1;
     int32_t     peer_idle_timeout = -1;
     int32_t     peer_latency = -1;
     int32_t     rcv_latency = -1;
     bool        tlpktdrop = false;
     bool        disconnected = false;
     bool        final_stats = false;
     bool        json_line = false;
     UString     json_prefix {};
     MilliSecond stats_interval = 0;
     SRTStatMode stats_mode = SRTStatMode::ALL;
private:
     // Callback which is called on any incoming connection.
     static int listenCallback(void* param, SRTSOCKET ns, int hsversion, const ::sockaddr* peeraddr, const char* streamid);
};


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SRTSocket::SRTSocket() :
    _guts(new Guts(this))
{
    CheckNonNull(_guts);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::SRTSocket::~SRTSocket(void)
{
    if (_guts != nullptr) {
        close(NULLREP);
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Basic getters (from guts).
//----------------------------------------------------------------------------

int ts::SRTSocket::getSocket() const
{
    return int(_guts->sock);
}

bool ts::SRTSocket::getMessageApi() const
{
    return _guts->messageapi;
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::open(SRTSocketMode mode,
                         const IPv4SocketAddress& local_address,
                         const IPv4SocketAddress& remote_address,
                         Report& report)
{
    // Filter already open condition.
    if (_guts->sock != SRT_INVALID_SOCK) {
        report.error(u"internal error, SRT socket already open");
        return false;
    }

    // Initialize socket modes.
    if (mode != SRTSocketMode::DEFAULT) {
        _guts->mode = mode;
        _guts->local_address = local_address;
        _guts->remote_address = remote_address;
    }
    _guts->disconnected = false;

    // Initialize SRT.
    SRTInit::Instance();

    // Create the SRT socket.
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 1)
    report.debug(u"calling srt_create_socket()");
    _guts->sock = ::srt_create_socket();
#else
    // Only supports IPv4.
    report.debug(u"calling srt_socket()");
    _guts->sock = ::srt_socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (_guts->sock == SRT_INVALID_SOCK) {
        report.error(u"error creating SRT socket: %s", {::srt_getlasterror_str()});
        return false;
    }

    // Set initial socket options.
    bool success = _guts->setSockOptPre(report);

    // Connect / setup the SRT socket.
    switch (_guts->mode) {
        case SRTSocketMode::LISTENER:
            success = success && _guts->srtListen(_guts->local_address, report);
            break;
        case SRTSocketMode::RENDEZVOUS:
            success = success &&
                _guts->srtBind(_guts->local_address, report) &&
                _guts->srtConnect(_guts->remote_address, report);
            break;
        case SRTSocketMode::CALLER:
            success = success &&
                (!_guts->local_address.hasAddress() || _guts->srtBind(_guts->local_address, report)) &&
                _guts->srtConnect(_guts->remote_address, report);
            break;
        case SRTSocketMode::DEFAULT:
        case SRTSocketMode::LEN:
        default:
            report.error(u"unsupported socket mode");
            success = false;
    }
    report.debug(u"SRTSocket::open, sock = 0x%X, listener = 0x%X", {_guts->sock, _guts->listener});

    // Set final socket options.
    success = success && _guts->setSockOptPost(report);

    // Reset send/receive statistics.
    _guts->total_sent_bytes = _guts->total_received_bytes = 0;
    if (_guts->stats_interval > 0) {
        _guts->next_stats = Time::CurrentUTC() + _guts->stats_interval;
    }

    if (!success) {
        close(report);
    }
    return success;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::close(Report& report)
{
    report.debug(u"SRTSocket::close, sock = 0x%X, listener = 0x%X, final stats: %s", {_guts->sock, _guts->listener, _guts->final_stats});

    // Report final statistics if required.
    if (_guts->final_stats) {
        // Sometimes, final statistics are not available, typically when the peer disconnected.
        // In that case, the SRT socket is in error state and it is no longer possible to get the stats.
        // This is an SRT bug since the final statistics should still be available as long as the socket is not closed.
        // See https://github.com/Haivision/srt/issues/2177
        reportStatistics(_guts->stats_mode, report);
    }

    // To handle the case where close() would be called from another thread,
    // clear the socket value first, then close.
    const ::SRTSOCKET sock = _guts->sock;
    const ::SRTSOCKET listener = _guts->listener;
    _guts->listener = SRT_INVALID_SOCK;
    _guts->sock = SRT_INVALID_SOCK;

    if (sock != SRT_INVALID_SOCK) {
        // Close the SRT data socket.
        report.debug(u"calling srt_close()");
        ::srt_close(sock);

        // Close the SRT listener socket if there is one.
        if (listener != SRT_INVALID_SOCK) {
            report.debug(u"calling srt_close() on listener socket");
            ::srt_close(listener);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Report statistics when necessary.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::reportStats(Report& report)
{
    bool status = true;
    if (stats_interval > 0) {
        const Time now(Time::CurrentUTC());
        if (now >= next_stats) {
            next_stats = now + stats_interval;
            status = _parent->reportStatistics(stats_mode, report);
        }
    }
    return status;
}


//----------------------------------------------------------------------------
// Check if the connection was disconnected by the peer.
//----------------------------------------------------------------------------

bool ts::SRTSocket::peerDisconnected() const
{
    return _guts->disconnected;
}


//----------------------------------------------------------------------------
// Preset local and remote socket addresses in string form.
//----------------------------------------------------------------------------

bool ts::SRTSocket::setAddressesInternal(const UString& listener_addr, const UString& caller_addr, const UString& local_addr, bool reset, Report& report)
{
    // Reset the addresses if needed.
    if (reset) {
        _guts->mode = SRTSocketMode::DEFAULT;
        _guts->local_address.clear();
        _guts->remote_address.clear();
    }

    // Nothing more than reset when neither listener nor caller are specified.
    if (caller_addr.empty() && listener_addr.empty()) {
        return true;
    }

    // Resolve communication mode.
    if (caller_addr.empty()) {
        _guts->mode = SRTSocketMode::LISTENER;
    }
    else if (listener_addr.empty()) {
        _guts->mode = SRTSocketMode::CALLER;
    }
    else {
        _guts->mode = SRTSocketMode::RENDEZVOUS;
    }

    // Local interface in caller mode.
    if (!local_addr.empty()) {
        if (!listener_addr.empty()) {
            report.error(u"specify either a listener address or a local outgoing interface for caller mode but not both");
            return false;
        }
        IPv4Address local_ip;
        if (!local_ip.resolve(local_addr, report)) {
            return false;
        }
        _guts->local_address.setAddress(local_ip);
        _guts->local_address.clearPort();
    }

    // Listener address, also used in rendezvous mode.
    if (!listener_addr.empty()) {
        if (!_guts->local_address.resolve(listener_addr, report)) {
            return false;
        }
        else if (!_guts->local_address.hasPort()) {
            report.error(u"missing port number in local listener address '%s'", {listener_addr});
            return false;
        }
    }

    // Caller address, also used in rendezvous mode.
    if (!caller_addr.empty()) {
        if (!_guts->remote_address.resolve(caller_addr, report)) {
            return false;
        }
        else if (!_guts->remote_address.hasAddress() || !_guts->remote_address.hasPort()) {
            report.error(u"missing address or port in remote caller address '%s'", {caller_addr});
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Load command line arguments.
//----------------------------------------------------------------------------

bool ts::SRTSocket::loadArgs(DuckContext& duck, Args& args)
{
    // Resolve caller/listener/rendezvous addresses.
    if (!setAddressesInternal(args.value(u"listener"), args.value(u"caller"), args.value(u"local-interface"), false, args)) {
        return false;
    }

    const UString ttype(args.value(u"transtype", u"live"));
    if (ttype != u"live" && ttype != u"file") {
        return false;
    }

    if (args.present(u"bufferapi") && args.present(u"messageapi")) {
        args.error(u"--bufferapi and --messageapi are mutually exclusive");
        return false;
    }

    _guts->transtype = (ttype == u"live") ? SRTT_LIVE : SRTT_FILE;
    _guts->enforce_encryption = args.present(u"enforce-encryption");
    _guts->messageapi = !args.present(u"bufferapi"); // --messageapi is now the default
    _guts->nakreport = args.present(u"nakreport");
    _guts->tlpktdrop = args.present(u"tlpktdrop");
    args.getIntValue(_guts->conn_timeout, u"conn-timeout", -1);
    args.getIntValue(_guts->ffs, u"ffs", -1);
    args.getIntValue(_guts->input_bw, u"input-bw", -1);
    args.getIntValue(_guts->iptos, u"iptos", -1);
    args.getIntValue(_guts->ipttl, u"ipttl", -1);
    args.getIntValue(_guts->kmrefreshrate, u"kmrefreshrate", -1);
    args.getIntValue(_guts->kmpreannounce, u"kmpreannounce", -1);
    args.getIntValue(_guts->latency, u"latency", -1);
    _guts->linger_opt.l_onoff = args.present(u"linger");
    _guts->reuse_port = !args.present(u"no-reuse-port");
    args.getIntValue(_guts->backlog, u"backlog", 1);
    args.getIntValue(_guts->linger_opt.l_linger, u"linger");
    args.getIntValue(_guts->lossmaxttl, u"lossmaxttl", -1);
    args.getIntValue(_guts->max_bw, u"max-bw", -1);
    args.getIntValue(_guts->min_version, u"min-version", -1);
    args.getIntValue(_guts->mss, u"mss", -1);
    args.getIntValue(_guts->ohead_bw, u"ohead-bw", -1);
    _guts->streamid = args.value(u"streamid").toUTF8();
    _guts->packet_filter = args.value(u"packet-filter").toUTF8();
    _guts->passphrase = args.value(u"passphrase").toUTF8();
    args.getIntValue(_guts->payload_size, u"payload-size", -1);
    args.getIntValue(_guts->pbkeylen, u"pbkeylen", -1);
    args.getIntValue(_guts->peer_idle_timeout, u"peer-idle-timeout", -1);
    args.getIntValue(_guts->peer_latency, u"peer-latency", -1);
    args.getIntValue(_guts->rcvbuf, u"rcvbuf", -1);
    args.getIntValue(_guts->rcv_latency, u"rcv-latency", -1);
    args.getIntValue(_guts->polling_time, u"polling-time", DEFAULT_POLLING_TIME);
    args.getIntValue(_guts->sndbuf, u"sndbuf", -1);
    args.getIntValue(_guts->udp_rcvbuf, u"udp-rcvbuf", -1);
    args.getIntValue(_guts->udp_sndbuf, u"udp-sndbuf", -1);
    args.getIntValue(_guts->stats_interval, u"statistics-interval", 0);
    _guts->final_stats = _guts->stats_interval > 0 || args.present(u"final-statistics");
    _guts->json_line = args.present(u"json-line");
    args.getValue(_guts->json_prefix, u"json-line");

    return true;
}


//----------------------------------------------------------------------------
// Set/get socket option.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::setSockOpt(int optName, const char* optNameStr, const void* optval, size_t optlen, Report& report)
{
    if (report.debug()) {
        report.debug(u"calling srt_setsockflag(%s, %s, %d)", {optNameStr, UString::Dump(optval, optlen, UString::SINGLE_LINE), optlen});
    }
    if (srt_setsockflag(sock, SRT_SOCKOPT(optName), optval, int(optlen)) < 0) {
        report.error(u"error during srt_setsockflag(%s): %s", {optNameStr, srt_getlasterror_str()});
        return false;
    }
    return true;
}

bool ts::SRTSocket::getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, Report& report) const
{
    report.debug(u"calling srt_getsockflag(%s, ..., %d)", {optNameStr, optlen});
    if (srt_getsockflag(_guts->sock, SRT_SOCKOPT(optName), optval, &optlen) < 0) {
        report.error(u"error during srt_getsockflag(%s): %s", {optNameStr, srt_getlasterror_str()});
        return false;
    }
    return true;
}

bool ts::SRTSocket::Guts::setSockOptPre(Report& report)
{
    const bool yes = true;

    if ((mode != SRTSocketMode::CALLER && !setSockOpt(SRTO_SENDER, "SRTO_SENDER", &yes, sizeof(yes), report)) ||
        (transtype != SRTT_INVALID && !setSockOpt(SRTO_TRANSTYPE, "SRTO_TRANSTYPE", &transtype, sizeof(transtype), report)) ||
        (!setSockOpt(SRTO_MESSAGEAPI, "SRTO_MESSAGEAPI", &messageapi, sizeof(messageapi), report)) ||
        (conn_timeout >= 0 && !setSockOpt(SRTO_CONNTIMEO, "SRTO_CONNTIMEO", &conn_timeout, sizeof(conn_timeout), report)) ||
        (mode == SRTSocketMode::RENDEZVOUS && !setSockOpt(SRTO_RENDEZVOUS, "SRTO_RENDEZVOUS", &yes, sizeof(yes), report)) ||
        (ffs > 0 && !setSockOpt(SRTO_FC, "SRTO_FC", &ffs, sizeof(ffs), report)) ||
        (iptos >= 0 && !setSockOpt(SRTO_IPTOS, "SRTO_IPTOS", &iptos, sizeof(iptos), report)) ||
        (ipttl > 0 && !setSockOpt(SRTO_IPTTL, "SRTO_IPTTL", &ipttl, sizeof(ipttl), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (enforce_encryption && !setSockOpt(SRTO_ENFORCEDENCRYPTION, "SRTO_ENFORCEDENCRYPTION", &enforce_encryption, sizeof(enforce_encryption), report)) ||
#endif
        (kmrefreshrate >= 0 && !setSockOpt(SRTO_KMREFRESHRATE, "SRTO_KMREFRESHRATE", &kmrefreshrate, sizeof(kmrefreshrate), report)) ||
        (kmpreannounce > 0 && !setSockOpt(SRTO_KMPREANNOUNCE, "SRTO_KMPREANNOUNCE", &kmpreannounce, sizeof(kmpreannounce), report)) ||
        (latency > 0 && !setSockOpt(SRTO_LATENCY, "SRTO_LATENCY", &latency, sizeof(latency), report)) ||
        (linger_opt.l_onoff && !setSockOpt(SRTO_LINGER, "SRTO_LINGER", &linger_opt, sizeof(linger_opt), report)) ||
        (lossmaxttl >= 0 && !setSockOpt(SRTO_LOSSMAXTTL, "SRTO_LOSSMAXTTL", &lossmaxttl, sizeof(lossmaxttl), report)) ||
        (max_bw >= 0 && !setSockOpt(SRTO_MAXBW, "SRTO_MAXBW", &max_bw, sizeof(max_bw), report)) ||
        (min_version > 0 && !setSockOpt(SRTO_MINVERSION, "SRTO_MINVERSION", &min_version, sizeof(min_version), report)) ||
        (mss >= 0 && !setSockOpt(SRTO_MSS, "SRTO_MSS", &mss, sizeof(mss), report)) ||
        (nakreport && !setSockOpt(SRTO_NAKREPORT, "SRTO_NAKREPORT", &nakreport, sizeof(nakreport), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (!packet_filter.empty() && !setSockOpt(SRTO_PACKETFILTER, "SRTO_PACKETFILTER", packet_filter.c_str(), int(packet_filter.size()), report)) ||
#endif
        (!passphrase.empty() && !setSockOpt(SRTO_PASSPHRASE, "SRTO_PASSPHRASE", passphrase.c_str(), int(passphrase.size()), report)) ||
        (!streamid.empty() && !setSockOpt(SRTO_STREAMID, "SRTO_STREAMID", streamid.c_str(), int(streamid.size()), report)) ||
        (payload_size > 0 && !setSockOpt(SRTO_PAYLOADSIZE, "SRTO_PAYLOADSIZE", &payload_size, sizeof(payload_size), report)) ||
        (pbkeylen > 0 && !setSockOpt(SRTO_PBKEYLEN, "SRTO_PBKEYLEN", &pbkeylen, sizeof(pbkeylen), report)) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 0)
        (peer_idle_timeout > 0 && !setSockOpt(SRTO_PEERIDLETIMEO, "SRTO_PEERIDLETIMEO", &peer_idle_timeout, sizeof(peer_idle_timeout), report)) ||
#endif
        (peer_latency > 0 && !setSockOpt(SRTO_PEERLATENCY, "SRTO_PEERLATENCY", &peer_latency, sizeof(peer_latency), report)) ||
        (rcvbuf > 0 && !setSockOpt(SRTO_RCVBUF, "SRTO_RCVBUF", &rcvbuf, sizeof(rcvbuf), report)) ||
        (rcv_latency > 0 && !setSockOpt(SRTO_RCVLATENCY, "SRTO_RCVLATENCY", &rcv_latency, sizeof(rcv_latency), report)) ||
        (sndbuf > 0 && !setSockOpt(SRTO_SNDBUF, "SRTO_SNDBUF", &sndbuf, sizeof(sndbuf), report)) ||
        (tlpktdrop && !setSockOpt(SRTO_TLPKTDROP, "SRTO_TLPKTDROP", &tlpktdrop, sizeof(tlpktdrop), report)))
    {
        return false;
    }

    // In case of error here, use system default
    if (udp_rcvbuf > 0) {
        setSockOpt(SRTO_UDP_RCVBUF, "SRTO_UDP_RCVBUF", &udp_rcvbuf, sizeof(udp_rcvbuf), report);
    }
    if (udp_sndbuf > 0) {
        setSockOpt(SRTO_UDP_SNDBUF, "SRTO_UDP_SNDBUF", &udp_sndbuf, sizeof(udp_sndbuf), report);
    }
    return true;
}

bool ts::SRTSocket::Guts::setSockOptPost(Report& report)
{
    if (max_bw == 0 && (
        (input_bw >= 0 && !setSockOpt(SRTO_INPUTBW, "SRTO_INPUTBW", &input_bw, sizeof(input_bw), report)) ||
        (ohead_bw >= 5 && !setSockOpt(SRTO_OHEADBW, "SRTO_OHEADBW", &ohead_bw, sizeof(ohead_bw), report))))
    {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Connection operation.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::srtListen(const IPv4SocketAddress& addr, Report& report)
{
    // The SRT socket will become the listener socket. As long as an error is possible, keep the
    // listener socket in "sock" field. On return false, this "sock" will ba closed by the caller.
    // On success, the listener socket must be moved in "listener" field and the "sock" field
    // will contain the client data socket.

    if (listener != SRT_INVALID_SOCK) {
        report.error(u"internal error, SRT listener socket already set");
        return false;
    }

    if (!setSockOpt(SRTO_REUSEADDR, "SRTO_REUSEADDR", &reuse_port, sizeof(reuse_port), report)) {
        return false;
    }

    ::sockaddr sock_addr;
    addr.copy(sock_addr);
    report.debug(u"calling srt_bind(%s)", {addr});
    if (::srt_bind(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        report.error(u"error during srt_bind(): %s", {::srt_getlasterror_str()});
        return false;
    }

    // Second parameter is the number of simultaneous connection accepted. For now we only accept one.
    report.debug(u"calling srt_listen()");
    if (::srt_listen(sock, backlog) < 0) {
        report.error(u"error during srt_listen(): %s", {::srt_getlasterror_str()});
        return false;
    }

    // Install a listen callback which will reject all subsequent connections after the first one.
    if (::srt_listen_callback(sock, listenCallback, this) < 0) {
        report.error(u"error during srt_listen_callback(): %s", {::srt_getlasterror_str()});
        return false;
    }

    // The original SRT socket becomes the listener SRT socket.
    ::sockaddr peer_addr;
    int peer_addr_len = sizeof(peer_addr);
    report.debug(u"calling srt_accept()");
    const int data_sock = ::srt_accept(sock, &peer_addr, &peer_addr_len);
    if (data_sock == SRT_INVALID_SOCK) {
        report.error(u"error during srt_accept(): %s", {::srt_getlasterror_str()});
        return false;
    }

    // Now keep the two SRT sockets in the context.
    listener = sock;
    sock = data_sock;

    // In listener mode, keep the address of the remote peer.
    const IPv4SocketAddress p_addr(peer_addr);
    report.debug(u"connected to %s", {p_addr});
    if (mode == SRTSocketMode::LISTENER) {
        remote_address = p_addr;
    }
    return true;
}

int ts::SRTSocket::Guts::listenCallback(void* param, SRTSOCKET sock, int hsversion, const ::sockaddr* peeraddr, const char* streamid)
{
    // Callback which is called on any incoming connection.
    // The first parameter is a pointer to the Guts instance.
    Guts* guts = reinterpret_cast<Guts*>(param);
    if (guts == nullptr || (guts->listener != SRT_INVALID_SOCK && guts->sock != SRT_INVALID_SOCK)) {
        // A connection is already established, revoke all others.
        #if defined(HAS_SRT_ACCESS_CONTROL)
            ::srt_setrejectreason(sock, SRT_REJX_OVERLOAD);
        #endif
        return -1;
    }
    else {
        // Initial connection accepted.
        return 0;
    }
}

bool ts::SRTSocket::Guts::srtConnect(const IPv4SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"calling srt_connect(%s)", {addr});
    if (::srt_connect(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        const int err = ::srt_getlasterror(&errno);
        std::string err_str(::srt_strerror(err, errno));
        if (err == SRT_ECONNREJ) {
            const RejectReason reason = ::srt_getrejectreason(sock);
            report.debug(u"srt_connect rejected, reason: %d", {reason});
#if defined(HAS_SRT_ACCESS_CONTROL)
            if (reason == SRT_REJX_OVERLOAD) {
                // Extended rejection reasons (REJX) have no meaningful error strings.
                // Since this one is expected, treat it differently.
                err_str.append(", server is overloaded, too many client connections already established");
            }
            else {
#endif
                err_str.append(", reject reason: ");
                err_str.append(::srt_rejectreason_str(reason));
#if defined(HAS_SRT_ACCESS_CONTROL)
            }
#endif
        }
        report.error(u"error during srt_connect: %s", {err_str});
        return false;
    }
    else {
        report.debug(u"srt_connect() successful");
        return true;
    }
}

bool ts::SRTSocket::Guts::srtBind(const IPv4SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"calling srt_bind(%s)", {addr});
    if (::srt_bind(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        report.error(u"error during srt_bind: %s", {::srt_getlasterror_str()});
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Send a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::SRTSocket::send(const void* data, size_t size, Report& report)
{
    return _guts->send(data, size, _guts->remote_address, report);
}

bool ts::SRTSocket::Guts::send(const void* data, size_t size, const IPv4SocketAddress& dest, Report& report)
{
    // If socket was disconnected or aborted, silently fail.
    if (disconnected || sock == SRT_INVALID_SOCK) {
        return false;
    }

    const int ret = ::srt_send(sock, reinterpret_cast<const char*>(data), int(size));
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        const int err = ::srt_getlasterror(nullptr);
        if (err == SRT_ECONNLOST || err == SRT_EINVSOCK) {
            disconnected = true;
        }
        else if (sock != SRT_INVALID_SOCK) {
            // Display error only if the socket was not closed in the meantime.
            report.error(u"error during srt_send(): %s", {::srt_getlasterror_str()});
        }
        return false;
    }

    total_sent_bytes += size;
    return reportStats(report);
}


//----------------------------------------------------------------------------
// Receive a message.
//----------------------------------------------------------------------------

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, Report& report)
{
    MicroSecond timestamp = 0; // unused
    return receive(data, max_size, ret_size, timestamp, report);
}

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, Report& report)
{
    ret_size = 0;
    timestamp = -1;

    // If socket was disconnected or aborted, silently fail.
    if (_guts->disconnected || _guts->sock == SRT_INVALID_SOCK) {
        return false;
    }

    // Message data
    ::SRT_MSGCTRL ctrl;
    TS_ZERO(ctrl);

    const int ret = ::srt_recvmsg2(_guts->sock, reinterpret_cast<char*>(data), int(max_size), &ctrl);
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        const int err = srt_getlasterror(nullptr);
        if (err == SRT_ECONNLOST || err == SRT_EINVSOCK) {
            _guts->disconnected = true;
        }
        else if (_guts->sock != SRT_INVALID_SOCK) {
            // Display error only if the socket was not closed in the meantime.
            report.error(u"error during srt_recv(): %s", {srt_getlasterror_str()});
        }
        return false;
    }
    if (ctrl.srctime != 0) {
        timestamp = MicroSecond(ctrl.srctime);
    }
    ret_size = size_t(ret);
    _guts->total_received_bytes += ret_size;
    return _guts->reportStats(report);
}


//----------------------------------------------------------------------------
// Send / receive statistics.
//----------------------------------------------------------------------------

size_t ts::SRTSocket::totalSentBytes() const
{
    return _guts->total_sent_bytes;
}

size_t ts::SRTSocket::totalReceivedBytes() const
{
    return _guts->total_received_bytes;
}


//----------------------------------------------------------------------------
// Get statistics about the socket and report them.
//----------------------------------------------------------------------------

bool ts::SRTSocket::reportStatistics(SRTStatMode mode, Report& report)
{
    // If socket was closed, silently fail.
    if (_guts->sock == SRT_INVALID_SOCK) {
        return false;
    }

    // Get statistics data from the SRT socket.
    // If the socket was disconnected but still open, the current version of libsrt cannot report statistics.
    // Let's try anyway in case some future version allows that but silently fails in case of error.
    ::SRT_TRACEBSTATS stats;
    TS_ZERO(stats);
    const int clear = (mode & SRTStatMode::INTERVAL) == SRTStatMode::NONE ? 0 : 1;

    if (::srt_bstats(_guts->sock, &stats, clear) < 0) {
        int sys_error = 0;
        const int srt_error = ::srt_getlasterror(&sys_error);
        report.debug(u"srt_bstats: socket: 0x%X, libsrt error: %d, system error: %d", {_guts->sock, srt_error, sys_error});
        if (!_guts->disconnected) {
            report.error(u"error during srt_bstats: %s", {::srt_getlasterror_str()});
        }
        return false;
    }

    // Build a statistics message.
    if (_guts->json_line) {
        // Statistics in JSON format.
        json::Object root;
        if ((mode & SRTStatMode::RECEIVE) != SRTStatMode::NONE) {
            root.query(u"receive.total", true).add(u"elapsed-ms", stats.msTimeStamp);
            root.query(u"receive.total", true).add(u"bytes", stats.byteRecvTotal);
            root.query(u"receive.total", true).add(u"packets", stats.pktRecvTotal);
            root.query(u"receive.total", true).add(u"lost-packets", stats.pktRcvLossTotal);
            root.query(u"receive.total", true).add(u"dropped-packets", stats.pktRcvDropTotal);
            // pktRcvRetransTotal to be added when available https://github.com/Haivision/srt/issues/1208
            // root.query(u"receive.total", true).add(u"retransmitted-packets", stats.pktRcvRetransTotal);
            root.query(u"receive.total", true).add(u"sent-ack-packets", stats.pktSentACKTotal);
            root.query(u"receive.total", true).add(u"sent-nak-packets", stats.pktSentNAKTotal);
            root.query(u"receive.total", true).add(u"undecrypted-packets", stats.pktRcvUndecryptTotal);
            root.query(u"receive.total", true).add(u"loss-bytes", stats.byteRcvLossTotal);
            root.query(u"receive.total", true).add(u"drop-bytes", stats.byteRcvDropTotal);
            root.query(u"receive.total", true).add(u"undecrypted-bytes", stats.byteRcvUndecryptTotal);
            root.query(u"receive.interval", true).add(u"rate-mbps", stats.mbpsRecvRate);
            root.query(u"receive.interval", true).add(u"bytes", stats.byteRecv);
            root.query(u"receive.interval", true).add(u"packets", stats.pktRecv);
            root.query(u"receive.interval", true).add(u"lost-packets", stats.pktRcvLoss);
            root.query(u"receive.interval", true).add(u"dropped-packets", stats.pktRcvDrop);
            root.query(u"receive.interval", true).add(u"retransmitted-packets", stats.pktRcvRetrans);
            root.query(u"receive.interval", true).add(u"sent-ack-packets", stats.pktSentACK);
            root.query(u"receive.interval", true).add(u"sent-nak-packets", stats.pktSentNAK);
            root.query(u"receive.interval", true).add(u"reorder-distance-packets", stats.pktReorderDistance);
            root.query(u"receive.interval", true).add(u"ignored-late-packets", stats.pktRcvBelated);
            root.query(u"receive.interval", true).add(u"undecrypted-packets", stats.pktRcvUndecrypt);
            root.query(u"receive.interval", true).add(u"loss-bytes", stats.byteRcvLoss);
            root.query(u"receive.interval", true).add(u"drop-bytes", stats.byteRcvDrop);
            root.query(u"receive.interval", true).add(u"undecrypted-bytes", stats.byteRcvUndecrypt);
            root.query(u"receive.instant", true).add(u"delivery-delay-ms", stats.msRcvTsbPdDelay);
            root.query(u"receive.instant", true).add(u"buffer-avail-bytes", stats.byteAvailRcvBuf);
            root.query(u"receive.instant", true).add(u"buffer-ack-packets", stats.pktRcvBuf);
            root.query(u"receive.instant", true).add(u"buffer-ack-bytes", stats.pktRcvBuf);
            root.query(u"receive.instant", true).add(u"buffer-ack-ms", stats.msRcvBuf);
            root.query(u"receive.instant", true).add(u"avg-belated-ms", stats.pktRcvAvgBelatedTime);
            root.query(u"receive.instant", true).add(u"mss-bytes", stats.byteMSS);
#if defined(SRT_VERSION_VALUE) && SRT_VERSION_VALUE >= SRT_MAKE_VERSION(1, 4, 0)
            root.query(u"receive.total", true).add(u"filter-extra-packets", stats.pktRcvFilterExtraTotal);
            root.query(u"receive.total", true).add(u"filter-recovered-packets", stats.pktRcvFilterSupplyTotal);
            root.query(u"receive.total", true).add(u"filter-not-recovered-packets", stats.pktRcvFilterLossTotal);
            root.query(u"receive.interval", true).add(u"filter-extra-packets", stats.pktRcvFilterExtra);
            root.query(u"receive.interval", true).add(u"filter-recovered-packets", stats.pktRcvFilterSupply);
            root.query(u"receive.interval", true).add(u"filter-not-recovered-packets", stats.pktRcvFilterLoss);
#endif
#if defined(SRT_VERSION_VALUE) && SRT_VERSION_VALUE >= SRT_MAKE_VERSION(1, 4, 1)
            root.query(u"receive.instant", true).add(u"reorder-tolerance-packets", stats.pktReorderTolerance);
#endif
#if defined(SRT_VERSION_VALUE) && SRT_VERSION_VALUE >= SRT_MAKE_VERSION(1, 4, 2)
            root.query(u"receive.total", true).add(u"unique-packets", stats.pktRecvUniqueTotal);
            root.query(u"receive.total", true).add(u"unique-bytes", stats.byteRecvUniqueTotal);
            root.query(u"receive.interval", true).add(u"unique-packets", stats.pktRecvUnique);
            root.query(u"receive.interval", true).add(u"unique-bytes", stats.byteRecvUnique);
#endif
        }
        if ((mode & SRTStatMode::SEND) != SRTStatMode::NONE) {
            root.query(u"send.total", true).add(u"elapsed-ms", stats.msTimeStamp);
            root.query(u"send.total", true).add(u"bytes", stats.byteSentTotal);
            root.query(u"send.total", true).add(u"packets", stats.pktSentTotal);
            root.query(u"send.total", true).add(u"retransmit-packets", stats.pktRetransTotal);
            root.query(u"send.total", true).add(u"lost-packets", stats.pktSndLossTotal);
            root.query(u"send.total", true).add(u"dropped-packets", stats.pktSndDropTotal);
            root.query(u"send.total", true).add(u"received-ack-packets", stats.pktRecvACKTotal);
            root.query(u"send.total", true).add(u"received-nak-packets", stats.pktRecvNAKTotal);
            root.query(u"send.total", true).add(u"send-duration-us", stats.usSndDurationTotal);
            root.query(u"send.total", true).add(u"restrans-bytes", stats.byteRetransTotal);
            root.query(u"send.total", true).add(u"drop-bytes", stats.byteSndDropTotal);
            root.query(u"send.interval", true).add(u"bytes", stats.byteSent);
            root.query(u"send.interval", true).add(u"packets", stats.pktSent);
            root.query(u"send.interval", true).add(u"retransmit-packets", stats.pktRetrans);
            root.query(u"send.interval", true).add(u"lost-packets", stats.pktSndLoss);
            root.query(u"send.interval", true).add(u"dropped-packets", stats.pktSndDrop);
            root.query(u"send.interval", true).add(u"received-ack-packets", stats.pktRecvACK);
            root.query(u"send.interval", true).add(u"received-nak-packets", stats.pktRecvNAK);
            root.query(u"send.interval", true).add(u"send-rate-mbps", stats.mbpsSendRate);
            root.query(u"send.interval", true).add(u"send-duration-us", stats.usSndDuration);
            root.query(u"send.interval", true).add(u"drop-bytes", stats.byteSndDrop);
            root.query(u"send.interval", true).add(u"retransmit-bytes", stats.byteRetrans);
            root.query(u"send.instant", true).add(u"delivery-delay-ms", stats.msSndTsbPdDelay);
            root.query(u"send.instant", true).add(u"interval-packets", stats.usPktSndPeriod);
            root.query(u"send.instant", true).add(u"flow-window-packets", stats.pktFlowWindow);
            root.query(u"send.instant", true).add(u"congestion-window-packets", stats.pktCongestionWindow);
            root.query(u"send.instant", true).add(u"in-flight-packets", stats.pktFlightSize);
            root.query(u"send.instant", true).add(u"estimated-link-bandwidth-mbps", stats.mbpsBandwidth);
            root.query(u"send.instant", true).add(u"avail-buffer-bytes", stats.byteAvailSndBuf);
            root.query(u"send.instant", true).add(u"max-bandwidth-mbps", stats.mbpsMaxBW);
            root.query(u"send.instant", true).add(u"mss-bytes", stats.byteMSS);
            root.query(u"send.instant", true).add(u"snd-buffer-packets", stats.pktSndBuf);
            root.query(u"send.instant", true).add(u"snd-buffer-bytes", stats.byteSndBuf);
            root.query(u"send.instant", true).add(u"snd-buffer-ms", stats.msSndBuf);
#if defined(SRT_VERSION_VALUE) && SRT_VERSION_VALUE >= SRT_MAKE_VERSION(1, 4, 0)
            root.query(u"send.total", true).add(u"filter-extra-packets", stats.pktSndFilterExtraTotal);
            root.query(u"send.interval", true).add(u"filter-extra-packets", stats.pktSndFilterExtra);
#endif
#if defined(SRT_VERSION_VALUE) && SRT_VERSION_VALUE >= SRT_MAKE_VERSION(1, 4, 2)
            root.query(u"send.total", true).add(u"unique-packets", stats.pktSentUniqueTotal);
            root.query(u"send.total", true).add(u"unique-bytes", stats.byteSentUniqueTotal);
            root.query(u"send.interval", true).add(u"unique-packets", stats.pktSentUnique);
            root.query(u"send.interval", true).add(u"unique-bytes", stats.byteSentUnique);
#endif
        }
        root.query(u"global.instant", true).add(u"rtt-ms", stats.msRTT);
        // Generate one line.
        report.info(_guts->json_prefix + root.oneLiner(report));
    }
    else {
        // Statistics in human-readable format.
        const bool show_receive = (_guts->total_received_bytes > 0 || stats.byteRecvTotal > 0) && (mode & SRTStatMode::RECEIVE) != SRTStatMode::NONE;
        const bool show_send = (_guts->total_sent_bytes > 0 || stats.byteSentTotal > 0) && (mode & SRTStatMode::SEND) != SRTStatMode::NONE;
        bool none = true;
        UString msg(u"SRT statistics:");
        if (show_receive && (mode & SRTStatMode::TOTAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Total received: %'d bytes, %'d packets, lost: %'d packets, dropped: %'d packets",
                       {stats.byteRecvTotal, stats.pktRecvTotal, stats.pktRcvLossTotal, stats.pktRcvDropTotal});
        }
        if (show_send && (mode & SRTStatMode::TOTAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Total sent: %'d bytes, %'d packets, retransmit: %'d packets, lost: %'d packets, dropped: %'d packets",
                       {stats.byteSentTotal, stats.pktSentTotal, stats.pktRetransTotal, stats.pktSndLossTotal, stats.pktSndDropTotal});
        }
        if (show_receive && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Interval received: %'d bytes, %'d packets, lost: %'d packets, dropped: %'d packets",
                       {stats.byteRecv, stats.pktRecv, stats.pktRcvLoss, stats.pktRcvDrop});
        }
        if (show_send && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Interval sent: %'d bytes, %'d packets, retransmit: %'d packets, lost: %'d packets, dropped: %'d packets",
                       {stats.byteSent, stats.pktSent, stats.pktRetrans, stats.pktSndLoss, stats.pktSndDrop});
        }
        if ((show_send || show_receive) && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.append(u"\n  Timestamp-based delivery delay");
            if (show_receive) {
                msg.format(u", receive: %d ms", {stats.msRcvTsbPdDelay});
            }
            if (show_send) {
                msg.format(u", send: %d ms", {stats.msSndTsbPdDelay});
            }
            msg.format(u", RTT: %f ms", {stats.msRTT});
        }
        if (none) {
            msg.append(u" none available");
        }
        report.info(msg);
    }

    return true;
}

#endif // TS_NO_SRT

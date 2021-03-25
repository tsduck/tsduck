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

#include "tsSRTSocket.h"
#include "tsSingletonManager.h"
#include "tsArgs.h"
#include "tsMutex.h"
#include "tsGuard.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Definition of command line arguments.
// These arguments are defined even in the absence of libsrt.
//----------------------------------------------------------------------------

void ts::SRTSocket::defineArgs(ts::Args& args) const
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
    args.help(u"linger", u"Linger time on close, recommended value: 0");

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

    args.option(u"messageapi");
    args.help(u"messageapi", u"When set, this socket uses the Message API, otherwise it uses Buffer API.");

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

    args.option(u"streamid", 0, Args::STRING);
    args.help(u"streamid",
              u"A string limited to 512 characters that can be set on the socket prior to connecting. "
              u"This stream ID will be able to be retrieved by the listener side from the socket that "
              u"is returned from srt_accept and was connected by a socket with that set stream ID (so "
              u"you usually use SET on the socket used for srt_connect and GET on the socket retrieved "
              u"from srt_accept). This string can be used completely free-form, however it's highly "
              u"recommended to follow the SRT Access Control guidlines.");

    args.option(u"udp-rcvbuf", 0, Args::POSITIVE);
    args.help(u"udp-rcvbuf", u"UDP Socket Receive Buffer Size.");

    args.option(u"udp-sndbuf", 0, Args::POSITIVE);
    args.help(u"udp-sndbuf", u"UDP Socket Send Buffer Size.");
}


//----------------------------------------------------------------------------
// Stubs in the absence of libsrt.
//----------------------------------------------------------------------------

#if defined(TS_NOSRT)

#define NOSRT_ERROR_MSG u"This version of TSDuck was compiled without SRT support"
#define NOSRT_ERROR { report.error(NOSRT_ERROR_MSG); return false; }

ts::SRTSocket::SRTSocket() : _guts(nullptr) {}
ts::SRTSocket::~SRTSocket() {}
bool ts::SRTSocket::open(SRTSocketMode mode, const ts::SocketAddress& local_addr, const ts::SocketAddress& remote_addr, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::close(ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::peerDisconnected() const { return false; }
bool ts::SRTSocket::loadArgs(ts::DuckContext& duck, ts::Args& args) { return true; }
bool ts::SRTSocket::send(const void* data, size_t size, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, ts::Report& report) NOSRT_ERROR
bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, ts::Report& report) NOSRT_ERROR
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
TS_LLVM_NOWARNING(undef)
TS_GCC_NOWARNING(undef)
TS_MSC_NOWARNING(4005)  // 'xxx' : macro redefinition
TS_MSC_NOWARNING(4668)  // 'xxx' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#include <srt/srt.h>
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
        void start();
        void stop();
    private:
        ts::Mutex _mutex;
        size_t    _count;
    };

    TS_DEFINE_SINGLETON(SRTInit);

    // Singleton constructor.
    SRTInit::SRTInit() :
        _mutex(),
        _count(0)
    {
    }

    // Called each time an SRT socket is opened.
    void SRTInit::start()
    {
        ts::Guard lock(_mutex);
        if (_count++ == 0) {
            srt_startup();
        }
    }

    // Called each time an SRT socket is closed.
    void SRTInit::stop()
    {
        ts::Guard lock(_mutex);
        if (_count > 0) {
            if (--_count == 0) {
                srt_cleanup();
            }
        }
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
     TS_NOCOPY(Guts);
public:
     // Default constructor.
     Guts();

     bool send(const void* data, size_t size, const SocketAddress& dest, Report& report);
     bool setSockOpt(int optName, const char* optNameStr, const void* optval, size_t optlen, Report& report);
     bool setSockOptPre(Report& report);
     bool setSockOptPost(Report& report);
     bool srtListen(const SocketAddress& addr, Report& report);
     bool srtConnect(const SocketAddress& addr, Report& report);
     bool srtBind(const SocketAddress& addr, Report& report);

     // Socket working data.
     SocketAddress local_address;
     SocketAddress remote_address;
     SRTSocketMode mode;
     volatile int  sock;       // SRT socket for data transmission
     volatile int  listener;   // Listener SRT socket when srt_listen() is used.

     // Socket options.
     SRT_TRANSTYPE transtype;
     std::string   packet_filter;
     std::string   passphrase;
     std::string   streamid;
     int     polling_time;
     bool    messageapi;
     bool    nakreport;
     int     conn_timeout;
     int     ffs;
     int     linger;
     int     lossmaxttl;
     int     mss;
     int     ohead_bw;
     int     payload_size;
     int     rcvbuf;
     int     sndbuf;
     bool    enforce_encryption;
     int32_t kmrefreshrate;
     int32_t kmpreannounce;
     int     udp_rcvbuf;
     int     udp_sndbuf;
     int64_t input_bw;
     int64_t max_bw;
     int32_t iptos;
     int32_t ipttl;
     int32_t latency;
     int32_t min_version;
     int32_t pbkeylen;
     int32_t peer_idle_timeout;
     int32_t peer_latency;
     int32_t rcv_latency;
     bool    tlpktdrop;
     bool    disconnected;
};


//----------------------------------------------------------------------------
// Guts constructor.
//----------------------------------------------------------------------------

ts::SRTSocket::Guts::Guts() :
    local_address(),
    remote_address(),
    mode(SRTSocketMode::DEFAULT),
    sock(-1),      // do not use SYS_SOCKET_INVALID, an SRT socket is not a socket, it is always an int
    listener(-1),  // idem
    transtype(SRTT_INVALID),
    packet_filter(),
    passphrase(),
    streamid(),
    polling_time(-1),
    messageapi(false),
    nakreport(false),
    conn_timeout(-1),
    ffs(-1),
    linger(-1),
    lossmaxttl(-1),
    mss(-1),
    ohead_bw(-1),
    payload_size(-1),
    rcvbuf(-1),
    sndbuf(-1),
    enforce_encryption(false),
    kmrefreshrate(-1),
    kmpreannounce(-1),
    udp_rcvbuf(-1),
    udp_sndbuf(-1),
    input_bw(-1),
    max_bw(-1),
    iptos(-1),
    ipttl(-1),
    latency(-1),
    min_version(-1),
    pbkeylen(-1),
    peer_idle_timeout(-1),
    peer_latency(-1),
    rcv_latency(-1),
    tlpktdrop(false),
    disconnected(false)
{
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SRTSocket::SRTSocket() :
    _guts(new Guts)
{
    CheckNonNull(_guts);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::SRTSocket::~SRTSocket(void)
{
    if (_guts != nullptr) {
        close();
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Basic getters (from guts).
//----------------------------------------------------------------------------

int ts::SRTSocket::getSocket() const
{
    return _guts->sock;
}

bool ts::SRTSocket::getMessageApi() const
{
    return _guts->messageapi;
}


//----------------------------------------------------------------------------
// Open the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SRTSocket::open(SRTSocketMode mode,
                         const ts::SocketAddress& local_address,
                         const ts::SocketAddress& remote_address,
                         ts::Report& report)
{
    // Filter already open condition.
    if (_guts->sock >= 0) {
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
    SRTInit::Instance()->start();

    // Create the SRT socket.
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 1)
    report.debug(u"calling srt_create_socket()");
    _guts->sock = srt_create_socket();
#else
    // Only supports IPv4.
    report.debug(u"calling srt_socket()");
    _guts->sock = srt_socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (_guts->sock < 0) {
        report.error(u"error creating SRT socket: %s", {srt_getlasterror_str()});
        SRTInit::Instance()->stop();
        return false;
    }

    // Set initial socket options.
    bool success = _guts->setSockOptPre(report);

    // Connect / setuo the SRT socket.
    switch (_guts->mode) {
        case SRTSocketMode::LISTENER:
            success = success && _guts->srtListen(_guts->local_address, report);
            break;
        case SRTSocketMode::RENDEZVOUS:
            success = success && _guts->srtBind(_guts->local_address, report) && _guts->srtConnect(_guts->remote_address, report);
            break;
        case SRTSocketMode::CALLER:
            success = success && _guts->srtConnect(_guts->remote_address, report);
            break;
        case SRTSocketMode::DEFAULT:
        case SRTSocketMode::LEN:
        default:
            report.error(u"unsupported socket mode");
            success = false;
    }

    // Set final socket options.
    success = success && _guts->setSockOptPost(report);

    if (!success) {
        close(report);
    }
    return success;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::close(ts::Report& report)
{
    // To handle the case where close() would be called from another thread,
    // clear the socket value first, then close.
    const int sock = _guts->sock;
    const int listener = _guts->listener;
    _guts->listener = -1;
    _guts->sock = -1;

    if (sock >= 0) {
        // Close the SRT data socket.
        report.debug(u"calling srt_close()");
        srt_close(sock);

        // Close the SRT listener socket if there is one.
        if (listener >= 0) {
            report.debug(u"calling srt_close() on listener socket");
            srt_close(listener);
        }

        // Decrement reference count to SRT library.
        SRTInit::Instance()->stop();
    }
    return true;
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

bool ts::SRTSocket::setAddressesInternal(const UString& local, const UString& remote, bool reset, Report& report)
{
    // Reset the addresses if needed.
    if (reset) {
        _guts->mode = SRTSocketMode::DEFAULT;
        _guts->local_address.clear();
        _guts->remote_address.clear();
    }

    // Select SRT mode.
    if (remote.empty() && local.empty()) {
        return true;  // unmodified
    }
    else if (remote.empty()) {
        _guts->mode = SRTSocketMode::LISTENER;
    }
    else if (local.empty()) {
        _guts->mode = SRTSocketMode::CALLER;
    }
    else {
        _guts->mode = SRTSocketMode::RENDEZVOUS;
    }

    // Resolve address strings.
    if (!local.empty()) {
        if (!_guts->local_address.resolve(local, report)) {
            return false;
        }
        else if (!_guts->local_address.hasPort()) {
            report.error(u"missing port number in local listener address '%s'", {local});
            return false;
        }
    }
    if (!remote.empty()) {
        if (!_guts->remote_address.resolve(remote, report)) {
            return false;
        }
        else if (!_guts->remote_address.hasAddress() || !_guts->remote_address.hasPort()) {
            report.error(u"missing address or port in remote caller address '%s'", {remote});
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Load command line arguments.
//----------------------------------------------------------------------------

bool ts::SRTSocket::loadArgs(ts::DuckContext& duck, ts::Args& args)
{
    // Resolve caller/listener/rendezvous addresses.
    if (!setAddressesInternal(args.value(u"listener"), args.value(u"caller"), false, args)) {
        return false;
    }

    const UString ttype(args.value(u"transtype", u"live"));
    if (ttype != u"live" && ttype != u"file") {
        return false;
    }

    _guts->transtype = (ttype == u"live") ? SRTT_LIVE : SRTT_FILE;
    _guts->nakreport = args.present(u"nakreport");
    _guts->conn_timeout = args.intValue<int>(u"conn-timeout", -1);
    _guts->messageapi = args.present(u"messageapi");
    _guts->ffs = args.intValue<int>(u"ffs", -1);
    _guts->input_bw = args.intValue<int64_t>(u"input-bw", -1);
    _guts->iptos = args.intValue<int32_t>(u"iptos", -1);
    _guts->ipttl = args.intValue<int32_t>(u"ipttl", -1);
    _guts->enforce_encryption = args.present(u"enforce-encryption");
    _guts->kmrefreshrate = args.intValue<int32_t>(u"kmrefreshrate", -1);
    _guts->kmpreannounce = args.intValue<int32_t>(u"kmpreannounce", -1);
    _guts->latency = args.intValue<int32_t>(u"latency", -1);
    _guts->linger = args.intValue<int>(u"linger", -1);
    _guts->lossmaxttl = args.intValue<int>(u"lossmaxttl", -1);
    _guts->max_bw = args.intValue<int64_t>(u"max-bw", -1);
    _guts->min_version = args.intValue<int32_t>(u"min-version", -1);
    _guts->mss = args.intValue<int>(u"mss", -1);
    _guts->ohead_bw = args.intValue<int>(u"ohead-bw", -1);
    _guts->streamid = args.value(u"streamid").toUTF8();
    _guts->packet_filter = args.value(u"packet-filter").toUTF8();
    _guts->passphrase = args.value(u"passphrase").toUTF8();
    _guts->payload_size = args.intValue<int>(u"payload-size", -1);
    _guts->pbkeylen = args.intValue<int32_t>(u"pbkeylen", -1);
    _guts->peer_idle_timeout = args.intValue<int32_t>(u"peer-idle-timeout", -1);
    _guts->peer_latency = args.intValue<int32_t>(u"peer-latency", -1);
    _guts->rcvbuf = args.intValue<int>(u"rcvbuf", -1);
    _guts->rcv_latency = args.intValue<int32_t>(u"rcv-latency", -1);
    _guts->polling_time = args.intValue<int>(u"polling-time", DEFAULT_POLLING_TIME);
    _guts->sndbuf = args.intValue<int>(u"sndbuf", -1);
    _guts->tlpktdrop = args.present(u"tlpktdrop");
    _guts->udp_rcvbuf = args.intValue<int>(u"udp-rcvbuf", -1);
    _guts->udp_sndbuf = args.intValue<int>(u"udp-sndbuf", -1);

    return true;
}


//----------------------------------------------------------------------------
// Set/get socket option.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::setSockOpt(int optName, const char* optNameStr, const void* optval, size_t optlen, ts::Report& report)
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

bool ts::SRTSocket::getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, ts::Report& report) const
{
    report.debug(u"calling srt_getsockflag(%s, ..., %d)", {optNameStr, optlen});
    if (srt_getsockflag(_guts->sock, SRT_SOCKOPT(optName), optval, &optlen) < 0) {
        report.error(u"error during srt_getsockflag(%s): %s", {optNameStr, srt_getlasterror_str()});
        return false;
    }
    return true;
}

bool ts::SRTSocket::Guts::setSockOptPre(ts::Report& report)
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
        (linger >= 0 && !setSockOpt(SRTO_LINGER, "SRTO_LINGER", &linger, sizeof(linger), report)) ||
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

bool ts::SRTSocket::Guts::srtListen(const SocketAddress& addr, Report& report)
{
    // The SRT socket will become the listener socket, check that there is none.
    if (listener >= 0) {
        report.error(u"internal error, SRT listener socket already set");
    }

    bool reuse = true;
    if (!setSockOpt(SRTO_REUSEADDR, "SRTO_REUSEADDR", &reuse, sizeof(reuse), report)) {
        return false;
    }

    ::sockaddr sock_addr;
    addr.copy(sock_addr);
    report.debug(u"calling srt_bind(%s)", {addr});
    if (srt_bind(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        report.error(u"error during srt_bind(): %s", {srt_getlasterror_str()});
        return false;
    }

    // Second parameter is the number of simultaneous connection accepted. For now we only accept one.
    report.debug(u"calling srt_listen()");
    if (srt_listen(sock, 1) < 0) {
        report.error(u"error during srt_listen(): %s", {srt_getlasterror_str()});
        return false;
    }

    // The original SRT socket becomes the listener SRT socket.
    ::sockaddr peer_addr;
    int peer_addr_len = sizeof(peer_addr);
    report.debug(u"calling srt_accept()");
    int data_sock = srt_accept(sock, &peer_addr, &peer_addr_len);
    if (data_sock < 0) {
        report.error(u"error during srt_accept(): %s", {srt_getlasterror_str()});
        return false;
    }

    // Now keep the two SRT sockets in the context.
    listener = sock;
    sock = data_sock;

    // In listener mode, keep the address of the remote peer.
    SocketAddress p_addr(peer_addr);
    report.debug(u"connected to %s", {p_addr});
    if (mode == SRTSocketMode::LISTENER) {
        remote_address = p_addr;
    }
    return true;
}

bool ts::SRTSocket::Guts::srtConnect(const ts::SocketAddress& addr, ts::Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"calling srt_connect(%s)", {addr});
    if (srt_connect(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        report.error(u"error during srt_connect: %s", {srt_getlasterror_str()});
        return false;
    }
    else {
        return true;
    }
}

bool ts::SRTSocket::Guts::srtBind(const ts::SocketAddress& addr, ts::Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"calling srt_bind(%s)", {addr});
    if (srt_bind(sock, &sock_addr, sizeof(sock_addr)) < 0) {
        report.error(u"error during srt_bind: %s", {srt_getlasterror_str()});
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Send a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::SRTSocket::send(const void* data, size_t size, ts::Report& report)
{
    return _guts->send(data, size, _guts->remote_address, report);
}

bool ts::SRTSocket::Guts::send(const void* data, size_t size, const ts::SocketAddress& dest, ts::Report& report)
{
    // If socket was disconnected or aborted, silently fail.
    if (disconnected || sock < 0) {
        return false;
    }

    const int ret = srt_send(sock, reinterpret_cast<const char*>(data), int(size));
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        if (srt_getlasterror(nullptr) == SRT_ECONNLOST) {
            disconnected = true;
        }
        else if (sock >= 0) {
            // Do not display error if the socket was closed in the meantime (sock < 0).
            report.error(u"error during srt_send(): %s", {srt_getlasterror_str()});
        }
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
    MicroSecond timestamp = 0; // unused
    return receive(data, max_size, ret_size, timestamp, report);
}

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, ts::Report& report)
{
    ret_size = 0;
    timestamp = -1;

    // If socket was disconnected or aborted, silently fail.
    if (_guts->disconnected || _guts->sock < 0) {
        return false;
    }

    // Message data
    ::SRT_MSGCTRL ctrl;
    TS_ZERO(ctrl);

    const int ret = srt_recvmsg2(_guts->sock, reinterpret_cast<char*>(data), int(max_size), &ctrl);
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        if (srt_getlasterror(nullptr) == SRT_ECONNLOST) {
            _guts->disconnected = true;
        }
        else if (_guts->sock >= 0) {
            // Do not display error if the socket was closed in the meantime (sock < 0).
            report.error(u"error during srt_recv(): %s", {srt_getlasterror_str()});
        }
        return false;
    }
    ret_size = size_t(ret);
    if (ctrl.srctime != 0) {
        timestamp = MicroSecond(ctrl.srctime);
    }
    return true;
}

#endif // TS_NOSRT

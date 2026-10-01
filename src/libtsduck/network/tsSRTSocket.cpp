//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2026, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSRTSocket.h"
#include "tsLibSRT.h"
#include "tsjsonObject.h"
#include "tsTime.h"
#include "tsMemory.h"
#include "tsCerrReport.h"
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_SRT)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"srt", u"SRT library", SUPPORT, ts::SRTSocket::GetLibraryVersion);


//----------------------------------------------------------------------------
// Stubs in the absence of libsrt.
//----------------------------------------------------------------------------

#if defined(TS_NO_SRT)

#define NOSRT_ERROR_MSG u"This version of TSDuck was compiled without SRT support"
#define NOSRT_ERROR { report().error(NOSRT_ERROR_MSG); return false; }

ts::SRTSocket::SRTSocket(Report* report) : ReporterBase(report), _guts(nullptr) {}
ts::SRTSocket::~SRTSocket() {}
bool ts::SRTSocket::isOpen() const { return false; }
bool ts::SRTSocket::open(SRTSocketMode, const IPSocketAddress&, const IPSocketAddress&) NOSRT_ERROR
bool ts::SRTSocket::close(bool silent) NOSRT_ERROR
bool ts::SRTSocket::getPeers(IPSocketAddress& local, IPSocketAddress& remote) NOSRT_ERROR
bool ts::SRTSocket::send(const void*, size_t) NOSRT_ERROR
bool ts::SRTSocket::receive(void*, size_t, size_t&) NOSRT_ERROR
bool ts::SRTSocket::receive(void*, size_t, size_t&, cn::microseconds&) NOSRT_ERROR
size_t ts::SRTSocket::totalSentBytes() const { return 0; }
size_t ts::SRTSocket::totalReceivedBytes() const { return 0; }
bool ts::SRTSocket::peerDisconnected() const { return false; }
bool ts::SRTSocket::reportStatistics(SRTStatMode) NOSRT_ERROR
bool ts::SRTSocket::getSockOpt(int, const char*, void*, int&) const NOSRT_ERROR
int  ts::SRTSocket::getSocket() const { return -1; }
ts::UString ts::SRTSocket::GetLibraryVersion() { return NOSRT_ERROR_MSG; }

#else


//----------------------------------------------------------------------------
// Actual libsrt implementation.
// A global singleton which initializes SRT.
// The SRT library is initialized when the first SRT socket is opened
// and terminated when the last socket is closed.
//----------------------------------------------------------------------------

namespace {
    class SRTInit
    {
        TS_SINGLETON(SRTInit);
    public:
        ~SRTInit();
    };

    TS_DEFINE_SINGLETON(SRTInit);

    // Singleton constructor, initialize SRT once.
    SRTInit::SRTInit()
    {
        CERR.debug(u"calling srt_startup()");
        const ts::Time start(ts::Time::CurrentUTC());
        ::srt_startup();
        const ts::Time end(ts::Time::CurrentUTC());
        CERR.debug(u"back from srt_startup(), %d ms", (end - start).count());
    }

    // Singleton destructor, cleanup SRT on application exit.
    SRTInit::~SRTInit()
    {
        CERR.debug(u"calling srt_cleanup()");
        const ts::Time start(ts::Time::CurrentUTC());
        ::srt_cleanup();
        const ts::Time end(ts::Time::CurrentUTC());
        CERR.debug(u"back from srt_cleanup(), %d ms", (end - start).count());
    }
}


//----------------------------------------------------------------------------
// Get the version of the SRT library.
//----------------------------------------------------------------------------

ts::UString ts::SRTSocket::GetLibraryVersion()
{
    UString version, library;

    // Initialize SRT.
    SRTInit::Instance();

    // Get the version from the dynamic library we have now.
    ::SRTSOCKET sock = ::srt_create_socket();
    if (sock != SRT_INVALID_SOCK) {
        int32_t value = 0;
        int len = sizeof(value);
        if (::srt_getsockflag(sock, SRTO_VERSION, &value, &len) == 0) {
            version.format(u"libsrt version %d.%d.%d", value >> 16, (value >> 8) & 0xFF, value & 0xFF);
            #if defined(ROBOTWEAX_SRT_VERSION_VALUE)
                // With Robotweax SRT, SRTO_VERSION gives the SRT compatibility version.
                // The version of the Robotweax SRT library is given by SRTO_ROBOTWEAX_VERSION.
                len = sizeof(value);
                if (::srt_getsockflag(sock, SRTO_ROBOTWEAX_VERSION, &value, &len) == 0) {
                    library.format(u"Robotweax SRT version %d.%d.%d", value >> 16, (value >> 8) & 0xFF, value & 0xFF);
                    #if ROBOTWEAX_SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(0, 2, 4)
                        // Starting with version 0.2.4, the cryptographic backend can be retrieved.
                        len = sizeof(value);
                        if (::srt_getsockflag(sock, SRTO_ROBOTWEAX_CRYPTO_BACKEND, &value, &len) == 0) {
                            static const std::map<int, const UChar*> backend_names {
                                {ROBOTWEAX_SRT_CRYPTO_BACKEND_OPENSSL, u"OpenSSL"},
                                {ROBOTWEAX_SRT_CRYPTO_BACKEND_BCRYPT,  u"Microsoft BCrypt"},
                            };
                            const auto it = backend_names.find(value);
                            if (it != backend_names.end()) {
                                library.format(u" with %s backend", it->second);
                            }
                            else {
                                library.format(u" with unknown backend #%d", value);
                            }
                        }
                    #endif
                }
            #endif
        }
        ::srt_close(sock);
    }

    // If failed to get version, just get the compiled version.
    if (version.empty()) {
        version = u"error getting libsrt version, compiled with version ";
        #if defined(SRT_VERSION_STRING)
            version.format(u"%s", SRT_VERSION_STRING);
        #else
            version.format(u"%d.%d.%d", SRT_VERSION_MAJOR, SRT_VERSION_MINOR, SRT_VERSION_PATCH);
        #endif
    }

    // Add the description of the library, if available.
    if (!library.empty()) {
        version.format(u" (%s)", library);
    }

    return version;
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

    bool send(const void* data, size_t size, const IPSocketAddress& dest);
    bool setSockOpt(int opt_name, const char* opt_name_str, const void* optval, size_t optlen);
    bool setSockOptPre();
    bool setSockOptPost();
    bool srtListen(const IPSocketAddress& addr);
    bool srtConnect(const IPSocketAddress& addr);
    bool srtBind(const IPSocketAddress& addr);
    bool reportStats();

    IPSocketAddress      remote_address {};             // Peer socket address.
    volatile ::SRTSOCKET sock = SRT_INVALID_SOCK;       // SRT socket for data transmission
    volatile ::SRTSOCKET listener  = SRT_INVALID_SOCK;  // Listener SRT socket when srt_listen() is used.
    bool                 disconnected = false;
    size_t               total_sent_bytes = 0;
    size_t               total_received_bytes = 0;
    Time                 next_stats {};
    ::SRT_TRANSTYPE      transtype = SRTT_INVALID;

private:
    // Callback which is called on any incoming connection.
    static int listenCallback(void* param, SRTSOCKET ns, int hsversion, const ::sockaddr* peeraddr, const char* streamid);
};


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SRTSocket::SRTSocket(Report* report) :
    ReporterBase(report),
    _guts(new Guts(this))
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::SRTSocket::~SRTSocket(void)
{
    if (_guts != nullptr) {
        close(true);
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Basic getters (from guts).
//----------------------------------------------------------------------------

bool ts::SRTSocket::isOpen() const
{
    return _guts->sock != SRT_INVALID_SOCK;
}

int ts::SRTSocket::getSocket() const
{
    return int(_guts->sock);
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::open(SRTSocketMode mode, const IPSocketAddress& local, const IPSocketAddress& remote)
{
    // Filter already open condition.
    if (_guts->sock != SRT_INVALID_SOCK) {
        report().error(u"internal error, SRT socket already open");
        return false;
    }

    // Initialize socket modes.
    if (mode != SRTSocketMode::DEFAULT) {
        _args.mode = mode;
        _args.local_address = local;
        _args.remote_address = remote;
    }
    _guts->transtype = _args.live_mode ? SRTT_LIVE : SRTT_FILE;
    _guts->disconnected = false;

    // The actual remote address is privately kept in guts, the actual value may vary in listener or rendezvous mode.
    _guts->remote_address = _args.remote_address;

    // Initialize SRT.
    SRTInit::Instance();

    // Create the SRT socket.
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 4, 1)
    report().debug(u"calling srt_create_socket()");
    _guts->sock = ::srt_create_socket();
#else
    // Only supports IPv4.
    report().debug(u"calling srt_socket()");
    _guts->sock = ::srt_socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (_guts->sock == SRT_INVALID_SOCK) {
        report().error(u"error creating SRT socket: %s", ::srt_getlasterror_str());
        return false;
    }

    // Set initial socket options.
    bool success = _guts->setSockOptPre();

    // Connect / setup the SRT socket.
    switch (_args.mode) {
        case SRTSocketMode::LISTENER:
            success = success && _guts->srtListen(_args.local_address);
            break;
        case SRTSocketMode::RENDEZVOUS:
            success = success &&
                _guts->srtBind(_args.local_address) &&
                _guts->srtConnect(_guts->remote_address);
            break;
        case SRTSocketMode::CALLER:
            success = success &&
                (!_args.local_address.hasAddress() || _guts->srtBind(_args.local_address)) &&
                _guts->srtConnect(_guts->remote_address);
            break;
        case SRTSocketMode::DEFAULT:
        default:
            report().error(u"unsupported socket mode");
            success = false;
    }
    report().debug(u"SRTSocket::open, sock = 0x%X, listener = 0x%X", _guts->sock, _guts->listener);

    // Set final socket options.
    success = success && _guts->setSockOptPost();

    // Reset send/receive statistics.
    _guts->total_sent_bytes = _guts->total_received_bytes = 0;
    if (_args.stats_interval > cn::milliseconds::zero()) {
        _guts->next_stats = Time::CurrentUTC() + _args.stats_interval;
    }

    if (!success) {
        close();
    }
    return success;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::SRTSocket::close(bool silent)
{
    report().debug(u"SRTSocket::close, sock = 0x%X, listener = 0x%X, final stats: %s", _guts->sock, _guts->listener, _args.final_stats);

    // Report final statistics if required.
    if (_args.final_stats) {
        // Sometimes, final statistics are not available, typically when the peer disconnected.
        // In that case, the SRT socket is in error state and it is no longer possible to get the stats.
        // This is an SRT bug since the final statistics should still be available as long as the socket is not closed.
        // See https://github.com/Haivision/srt/issues/2177
        reportStatistics(_args.stats_mode);
    }

    // To handle the case where close() would be called from another thread,
    // clear the socket value first, then close.
    const ::SRTSOCKET sock = _guts->sock;
    const ::SRTSOCKET listener = _guts->listener;
    _guts->listener = SRT_INVALID_SOCK;
    _guts->sock = SRT_INVALID_SOCK;

    if (sock != SRT_INVALID_SOCK) {
        // Close the SRT data socket.
        report().debug(u"calling srt_close()");
        ::srt_close(sock);

        // Close the SRT listener socket if there is one.
        if (listener != SRT_INVALID_SOCK) {
            report().debug(u"calling srt_close() on listener socket");
            ::srt_close(listener);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Get the socket peers, local and remote.
//----------------------------------------------------------------------------

bool ts::SRTSocket::getPeers(IPSocketAddress& local, IPSocketAddress& remote)
{
    ::sockaddr_storage addr;
    int addr_len = sizeof(addr);

    // Get local socket.
    report().debug(u"calling srt_getsockname()");
    if (::srt_getsockname(_guts->sock, reinterpret_cast<::sockaddr*>(&addr), &addr_len) < 0) {
        report().error(u"error during srt_getsockname(): %s", ::srt_getlasterror_str());
        return false;
    }
    local.set(addr);

    // Get remote socket.
    addr_len = sizeof(addr);
    report().debug(u"calling srt_getpeername()");
    if (::srt_getpeername(_guts->sock, reinterpret_cast<::sockaddr*>(&addr), &addr_len) < 0) {
        report().error(u"error during srt_getpeername(): %s", ::srt_getlasterror_str());
        return false;
    }
    remote.set(addr);
    return true;
}


//----------------------------------------------------------------------------
// Report statistics when necessary.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::reportStats()
{
    bool status = true;
    if (_parent->_args.stats_interval > cn::milliseconds::zero()) {
        const Time now(Time::CurrentUTC());
        if (now >= next_stats) {
            next_stats = now + _parent->_args.stats_interval;
            status = _parent->reportStatistics(_parent->_args.stats_mode);
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
// Set/get one socket option.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::setSockOpt(int opt_name, const char* opt_name_str, const void* optval, size_t optlen)
{
    if (_parent->report().debug()) {
        _parent->report().debug(u"calling srt_setsockflag(%s, %s, %d)", opt_name_str, UString::Dump(optval, optlen, UString::SINGLE_LINE), optlen);
    }
    if (::srt_setsockflag(sock, SRT_SOCKOPT(opt_name), optval, int(optlen)) < 0) {
        _parent->report().error(u"error during srt_setsockflag(%s): %s", opt_name_str, ::srt_getlasterror_str());
        return false;
    }
    return true;
}

bool ts::SRTSocket::getSockOpt(int opt_name, const char* opt_names_str, void* optval, int& optlen) const
{
    report().debug(u"calling srt_getsockflag(%s, ..., %d)", opt_names_str, optlen);
    if (::srt_getsockflag(_guts->sock, SRT_SOCKOPT(opt_name), optval, &optlen) < 0) {
        report().error(u"error during srt_getsockflag(%s): %s", opt_names_str, ::srt_getlasterror_str());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Configure all socket options.
//----------------------------------------------------------------------------

// Temporary macros to simplify calls to setSockOpt() when a condition is true.
// The result evals to true in case of success (either not condition or successful setSockOpt).
#define COND_SETOPT(cond, name, value) (!(cond) || setSockOpt(name, #name, &(value), sizeof(value)))
#define COND_SETOPT_STR(cond, name, value) (!(cond) || setSockOpt(name, #name, (value).c_str(), (value).size()))

// Set pre-options, before listen/bind/connect.
bool ts::SRTSocket::Guts::setSockOptPre()
{
    const auto& a = _parent->_args;
    const bool yes = true;

    // Does SRT API use UTF-8 strings?.
    std::string u8_congestion(a.congestion.toUTF8());
    std::string u8_packet_filter(a.packet_filter.toUTF8());
    std::string u8_passphrase(a.passphrase.toUTF8());
    std::string u8_stream_id(a.stream_id.toUTF8());

    // SRT API needs milliseconds in int32_t variables.
    int32_t conn_timeout = int32_t(a.connection_timeout.count());
    int32_t latency = int32_t(a.latency.count());
    int32_t peer_latency = int32_t(a.peer_latency.count());
    int32_t rcv_latency = int32_t(a.rcv_latency.count());
    int32_t peer_idle_timeout = int32_t(a.peer_idle_timeout.count());
    int32_t groupminstabletimeo = int32_t(a.groupminstabletimeo.count());

    // We require at least SRT 1.5.0 now. We only condition more recent options.

    if (!COND_SETOPT(a.mode != SRTSocketMode::CALLER, SRTO_SENDER, yes) ||
        !COND_SETOPT(true, SRTO_REUSEADDR, a.reuse_port) ||
        !COND_SETOPT(transtype != SRTT_INVALID, SRTO_TRANSTYPE, transtype) ||
        !COND_SETOPT_STR(!u8_congestion.empty(), SRTO_CONGESTION, u8_congestion) ||
        !COND_SETOPT(a.message_api.has_value(), SRTO_MESSAGEAPI, *a.message_api) ||
        !COND_SETOPT(a.nakreport.has_value(), SRTO_NAKREPORT, *a.nakreport) ||
        !COND_SETOPT(a.tlpktdrop.has_value(), SRTO_TLPKTDROP, *a.tlpktdrop) ||
        !COND_SETOPT(a.tsbpdmode.has_value(), SRTO_TSBPDMODE, *a.tsbpdmode) ||
        !COND_SETOPT(conn_timeout >= 0, SRTO_CONNTIMEO, conn_timeout) ||
        !COND_SETOPT(a.mode == SRTSocketMode::RENDEZVOUS, SRTO_RENDEZVOUS, yes) ||
        !COND_SETOPT(a.payload_size >= 0, SRTO_PAYLOADSIZE, a.payload_size) ||
        !COND_SETOPT(a.fc_packets >= 0, SRTO_FC, a.fc_packets) ||
        !COND_SETOPT(a.iptos >= 0, SRTO_IPTOS, a.iptos) ||
        !COND_SETOPT(a.ipttl > 0, SRTO_IPTTL, a.ipttl) ||
        !COND_SETOPT(a.enforce_encryption, SRTO_ENFORCEDENCRYPTION, yes) ||
        !COND_SETOPT(a.kmrefreshrate >= 0, SRTO_KMREFRESHRATE, a.kmrefreshrate) ||
        !COND_SETOPT(a.kmpreannounce > 0, SRTO_KMPREANNOUNCE, a.kmpreannounce) ||
        !COND_SETOPT(a.linger_opt.l_onoff, SRTO_LINGER, a.linger_opt) ||
        !COND_SETOPT(a.min_version > 0, SRTO_MINVERSION, a.min_version) ||
        !COND_SETOPT(a.mss >= 0, SRTO_MSS, a.mss) ||
        !COND_SETOPT_STR(!u8_stream_id.empty(), SRTO_STREAMID, u8_stream_id) ||
        !COND_SETOPT_STR(!u8_packet_filter.empty(), SRTO_PACKETFILTER, u8_packet_filter) ||
        !COND_SETOPT_STR(!u8_passphrase.empty(), SRTO_PASSPHRASE, u8_passphrase) ||
        !COND_SETOPT(a.pbkeylen >= 0, SRTO_PBKEYLEN, a.pbkeylen) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 5, 2)
        !COND_SETOPT(a.crypto_mode >= 0, SRTO_CRYPTOMODE, a.crypto_mode) ||
#endif
        !COND_SETOPT(a.group_connect >= 0, SRTO_GROUPCONNECT, a.group_connect) ||
        !COND_SETOPT(groupminstabletimeo >= 0, SRTO_GROUPMINSTABLETIMEO, a.groupminstabletimeo) ||
        !COND_SETOPT(a.retransmit_algo >= 0, SRTO_RETRANSMITALGO, a.retransmit_algo) ||
        !COND_SETOPT(latency >= 0, SRTO_LATENCY, latency) ||
        !COND_SETOPT(rcv_latency >= 0, SRTO_RCVLATENCY, rcv_latency) ||
        !COND_SETOPT(peer_latency >= 0, SRTO_PEERLATENCY, peer_latency) ||
        !COND_SETOPT(peer_idle_timeout >= 0, SRTO_PEERIDLETIMEO, peer_idle_timeout) ||
        !COND_SETOPT(a.ipv6_only >= -1, SRTO_IPV6ONLY, a.ipv6_only) ||
        !COND_SETOPT(a.rcvbuf > 0, SRTO_RCVBUF, a.rcvbuf) ||
        !COND_SETOPT(a.sndbuf > 0, SRTO_SNDBUF, a.sndbuf))
    {
        return false;
    }

    // In case of error here, use system default, don't fail.
    COND_SETOPT(a.udp_rcvbuf > 0, SRTO_UDP_RCVBUF, a.udp_rcvbuf);
    COND_SETOPT(a.udp_sndbuf > 0, SRTO_UDP_SNDBUF, a.udp_sndbuf);

    return true;
}

// Set post-options, after listen/bind/connect.
bool ts::SRTSocket::Guts::setSockOptPost()
{
    const auto& a = _parent->_args;

    // SRT API needs milliseconds in int32_t variables.
    int32_t snd_drop_delay = int32_t(a.snd_drop_delay.count());

    if (!COND_SETOPT(a.drift_tracer.has_value(), SRTO_DRIFTTRACER, *a.drift_tracer) ||
        !COND_SETOPT(a.lossmaxttl >= 0, SRTO_LOSSMAXTTL, a.lossmaxttl) ||
        !COND_SETOPT(a.max_bw >= -1, SRTO_MAXBW, a.max_bw) ||
        !COND_SETOPT(a.input_bw >= 0, SRTO_INPUTBW, a.input_bw) ||
        !COND_SETOPT(a.min_input_bw >= 0, SRTO_MININPUTBW, a.min_input_bw) ||
#if SRT_VERSION_VALUE >= SRT_MAKE_VERSION_VALUE(1, 5, 3) && !defined(ROBOTWEAX_SRT_VERSION_VALUE)
        !COND_SETOPT(a.max_rexmit_bw >= -1, SRTO_MAXREXMITBW, a.max_rexmit_bw) ||
#endif
        !COND_SETOPT(a.ohead_bw >= 5, SRTO_OHEADBW, a.ohead_bw) ||
        !COND_SETOPT(snd_drop_delay >= -1, SRTO_SNDDROPDELAY, snd_drop_delay))
    {
        return false;
    }

    return true;
}

// End of temporary macros.
#undef COND_SETOPT_STR
#undef COND_SETOPT


//----------------------------------------------------------------------------
// Connection operation.
//----------------------------------------------------------------------------

bool ts::SRTSocket::Guts::srtListen(const IPSocketAddress& addr)
{
     const auto& args = _parent->_args;

    // The SRT socket will become the listener socket. As long as an error is possible, keep the
    // listener socket in "sock" field. On return false, this "sock" will ba closed by the caller.
    // On success, the listener socket must be moved in "listener" field and the "sock" field
    // will contain the client data socket.

    if (listener != SRT_INVALID_SOCK) {
        _parent->report().error(u"internal error, SRT listener socket already set");
        return false;
    }

    ::sockaddr_storage sock_addr;
    const size_t sock_size = addr.get(sock_addr);
    _parent->report().debug(u"calling srt_bind(%s)", addr);
    if (::srt_bind(sock, reinterpret_cast<const ::sockaddr*>(&sock_addr), int(sock_size)) < 0) {
        _parent->report().error(u"error during srt_bind(): %s", ::srt_getlasterror_str());
        return false;
    }

    // Install a listen callback which will reject all subsequent connections after the first one.
    _parent->report().debug(u"calling srt_listen_callback()");
    if (::srt_listen_callback(sock, listenCallback, this) < 0) {
        _parent->report().error(u"error during srt_listen_callback(): %s", ::srt_getlasterror_str());
        return false;
    }

    // Second parameter is the number of simultaneous connection accepted. For now we only accept one.
    _parent->report().debug(u"calling srt_listen()");
    if (::srt_listen(sock, args.backlog) < 0) {
        _parent->report().error(u"error during srt_listen(): %s", ::srt_getlasterror_str());
        return false;
    }

    // The original SRT socket becomes the listener SRT socket.
    ::sockaddr_storage peer_addr;
    int peer_addr_len = sizeof(peer_addr);
    _parent->report().debug(u"calling srt_accept()");
    const int data_sock = ::srt_accept(sock, reinterpret_cast<::sockaddr*>(&peer_addr), &peer_addr_len);
    if (data_sock == SRT_INVALID_SOCK) {
        _parent->report().error(u"error during srt_accept(): %s", ::srt_getlasterror_str());
        return false;
    }

    // Now keep the two SRT sockets in the context.
    listener = sock;
    sock = data_sock;

    // In listener mode, keep the address of the remote peer.
    const IPSocketAddress rem_addr(peer_addr);
    _parent->report().debug(u"connected to %s", rem_addr);
    if (args.mode == SRTSocketMode::LISTENER) {
        remote_address = rem_addr;
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

bool ts::SRTSocket::Guts::srtConnect(const IPSocketAddress& addr)
{
    ::sockaddr_storage sock_addr;
    const size_t sock_size = addr.get(sock_addr);

    _parent->report().debug(u"calling srt_connect(%s)", addr);
    if (::srt_connect(sock, reinterpret_cast<const ::sockaddr*>(&sock_addr), int(sock_size)) < 0) {
        const int err = ::srt_getlasterror(&errno);
        std::string err_str(::srt_strerror(err, errno));
        if (err == SRT_ECONNREJ) {
            _parent->report().debug(u"calling srt_getrejectreason()");
            const SRT_RejectReason reason = ::srt_getrejectreason(sock);
            _parent->report().debug(u"srt_connect rejected, reason: %d", reason);
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
        _parent->report().error(u"error during srt_connect: %s", err_str);
        return false;
    }
    else {
        _parent->report().debug(u"srt_connect() successful");
        return true;
    }
}

bool ts::SRTSocket::Guts::srtBind(const IPSocketAddress& addr)
{
    ::sockaddr_storage sock_addr;
    const size_t sock_size = addr.get(sock_addr);

    if (addr.generation() == IP::v6) {
        // The SRT API doc says that "When you bind an IPv6 wildcard address, note that the SRTO_IPV6ONLY
        // option must be set on the socket explicitly to 1 or 0 prior to calling this function."
        int32_t opt = 0;
        if (!setSockOpt(SRTO_IPV6ONLY, "SRTO_IPV6ONLY", &opt, sizeof(opt))) {
            return false;
        }
    }

    _parent->report().debug(u"calling srt_bind(%s)", addr);
    if (::srt_bind(sock, reinterpret_cast<const ::sockaddr*>(&sock_addr), int(sock_size)) < 0) {
        _parent->report().error(u"error during srt_bind: %s", ::srt_getlasterror_str());
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Send a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::SRTSocket::send(const void* data, size_t size)
{
    return _guts->send(data, size, _guts->remote_address);
}

bool ts::SRTSocket::Guts::send(const void* data, size_t size, const IPSocketAddress& dest)
{
    // If socket was disconnected or aborted, silently fail.
    if (disconnected || sock == SRT_INVALID_SOCK) {
        return false;
    }

    _parent->report().log(2, u"calling srt_send(), %d bytes", size);
    const int ret = ::srt_send(sock, reinterpret_cast<const char*>(data), int(size));
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        const int err = ::srt_getlasterror(nullptr);
        if (err == SRT_ECONNLOST || err == SRT_EINVSOCK) {
            disconnected = true;
        }
        else if (sock != SRT_INVALID_SOCK) {
            // Display error only if the socket was not closed in the meantime.
            _parent->report().error(u"error during srt_send(): %s", ::srt_getlasterror_str());
        }
        return false;
    }

    total_sent_bytes += size;
    return reportStats();
}


//----------------------------------------------------------------------------
// Receive a message.
//----------------------------------------------------------------------------

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size)
{
    cn::microseconds timestamp {}; // unused
    return receive(data, max_size, ret_size, timestamp);
}

bool ts::SRTSocket::receive(void* data, size_t max_size, size_t& ret_size, cn::microseconds& timestamp)
{
    ret_size = 0;
    timestamp = cn::microseconds(-1);

    // If socket was disconnected or aborted, silently fail.
    if (_guts->disconnected || _guts->sock == SRT_INVALID_SOCK) {
        return false;
    }

    // Message data
    ::SRT_MSGCTRL ctrl;
    TS_ZERO(ctrl);

    report().log(2, u"calling srt_recvmsg2(), buffer size: %d bytes", max_size);
    const int ret = ::srt_recvmsg2(_guts->sock, reinterpret_cast<char*>(data), int(max_size), &ctrl);
    if (ret < 0) {
        // Differentiate peer disconnection (aka "end of file") and actual errors.
        const int err = ::srt_getlasterror(nullptr);
        if (err == SRT_ECONNLOST || err == SRT_EINVSOCK) {
            _guts->disconnected = true;
        }
        else if (_guts->sock != SRT_INVALID_SOCK) {
            // Display error only if the socket was not closed in the meantime.
            report().error(u"error during srt_recv(): %s", ::srt_getlasterror_str());
        }
        return false;
    }
    if (ctrl.srctime != 0) {
        timestamp = cn::microseconds(cn::microseconds::rep(ctrl.srctime));
    }
    ret_size = size_t(ret);
    report().log(2, u"srt_recvmsg2(), received %d bytes", ret_size);
    _guts->total_received_bytes += ret_size;
    return _guts->reportStats();
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

bool ts::SRTSocket::reportStatistics(SRTStatMode mode)
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

    report().log(2, u"calling srt_bstats()");
    if (::srt_bstats(_guts->sock, &stats, clear) < 0) {
        int sys_error = 0;
        const int srt_error = ::srt_getlasterror(&sys_error);
        report().debug(u"srt_bstats: socket: 0x%X, libsrt error: %d, system error: %d", _guts->sock, srt_error, sys_error);
        if (!_guts->disconnected) {
            report().error(u"error during srt_bstats: %s", ::srt_getlasterror_str());
        }
        return false;
    }

    // Build a statistics message.
    if (_args.json_line) {
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
        report().info(_args.json_prefix + root.oneLiner());
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
                       stats.byteRecvTotal, stats.pktRecvTotal, stats.pktRcvLossTotal, stats.pktRcvDropTotal);
        }
        if (show_send && (mode & SRTStatMode::TOTAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Total sent: %'d bytes, %'d packets, retransmit: %'d packets, lost: %'d packets, dropped: %'d packets",
                       stats.byteSentTotal, stats.pktSentTotal, stats.pktRetransTotal, stats.pktSndLossTotal, stats.pktSndDropTotal);
        }
        if (show_receive && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Interval received: %'d bytes, %'d packets, lost: %'d packets, dropped: %'d packets",
                       stats.byteRecv, stats.pktRecv, stats.pktRcvLoss, stats.pktRcvDrop);
        }
        if (show_send && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.format(u"\n  Interval sent: %'d bytes, %'d packets, retransmit: %'d packets, lost: %'d packets, dropped: %'d packets",
                       stats.byteSent, stats.pktSent, stats.pktRetrans, stats.pktSndLoss, stats.pktSndDrop);
        }
        if ((show_send || show_receive) && (mode & SRTStatMode::INTERVAL) != SRTStatMode::NONE) {
            none = false;
            msg.append(u"\n  Timestamp-based delivery delay");
            if (show_receive) {
                msg.format(u", receive: %d ms", stats.msRcvTsbPdDelay);
            }
            if (show_send) {
                msg.format(u", send: %d ms", stats.msSndTsbPdDelay);
            }
            msg.format(u", RTT: %f ms", stats.msRTT);
        }
        if (none) {
            msg.append(u" none available");
        }
        report().info(msg);
    }

    return true;
}

#endif // TS_NO_SRT

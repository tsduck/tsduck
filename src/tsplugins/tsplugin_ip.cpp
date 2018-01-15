//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  IP input / output
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsIPUtils.h"
#include "tsUDPSocket.h"
#include "tsSysUtils.h"
#include "tsTime.h"
TSDUCK_SOURCE;

// Grouping TS packets in UDP packets

#define DEF_PACKET_BURST     7  // 1316 B, fits (with headers) in Ethernet MTU
#define MAX_PACKET_BURST   128  // ~ 48 kB
#define MAX_IP_SIZE      65536


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {

    // Input plugin
    class IPInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        IPInput(TSP*);
        virtual ~IPInput();
        virtual bool start() override;
        virtual bool stop() override;
        virtual BitRate getBitrate() override;
        virtual size_t receive(TSPacket*, size_t) override;

    private:
        UDPSocket     _sock;               // Incoming socket
        MilliSecond   _eval_time;          // Bitrate evaluation interval in milli-seconds
        MilliSecond   _display_time;       // Bitrate display interval in milli-seconds
        Time          _next_display;       // Next bitrate display time
        Time          _start;              // UTC date of first received packet
        PacketCounter _packets;            // Number of received packets since _start
        Time          _start_0;            // Start of previous bitrate evaluation period
        PacketCounter _packets_0;          // Number of received packets since _start_0
        Time          _start_1;            // Start of previous bitrate evaluation period
        PacketCounter _packets_1;          // Number of received packets since _start_1
        size_t        _inbuf_count;        // Remaining TS packets in inbuf
        size_t        _inbuf_next;         // Index in inbuf of next TS packet to return
        uint8_t       _inbuf[MAX_IP_SIZE]; // Input buffer

        // Inaccessible operations
        IPInput() = delete;
        IPInput(const IPInput&) = delete;
        IPInput& operator=(const IPInput&) = delete;
    };

    // Output plugin
    class IPOutput: public OutputPlugin
    {
    public:
        // Implementation of plugin API
        IPOutput(TSP*);
        virtual ~IPOutput();
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, size_t) override;

    private:
        UDPSocket _sock;        // Outgoing socket
        size_t    _pkt_burst;   // Number of TS packets per UDP message

        // Inaccessible operations
        IPOutput() = delete;
        IPOutput(const IPOutput&) = delete;
        IPOutput& operator=(const IPOutput&) = delete;
     };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(ts::IPInput)
TSPLUGIN_DECLARE_OUTPUT(ts::IPOutput)


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::IPInput::IPInput(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from UDP/IP, multicast or unicast.", u"[options] [address:]port"),
    _sock(false, *tsp_),
    _eval_time(0),
    _display_time(0),
    _next_display(Time::Epoch),
    _start(Time::Epoch),
    _packets(0),
    _start_0(Time::Epoch),
    _packets_0(0),
    _start_1(Time::Epoch),
    _packets_1(0),
    _inbuf_count(0),
    _inbuf_next(0),
    _inbuf()
{
    option(u"",                     0,  STRING, 1, 1);
    option(u"buffer-size",         'b', UNSIGNED);
    option(u"display-interval",    'd', POSITIVE);
    option(u"evaluation-interval", 'e', POSITIVE);
    option(u"local-address",       'l', STRING);
    option(u"reuse-port",          'r');

    setHelp(u"Parameter:\n"
            u"  The parameter [address:]port describes the destination of UDP packets.\n"
            u"  The 'port' part is mandatory and specifies the UDP port to listen on.\n"
            u"  The 'address' part is optional. It specifies an IP multicast address\n"
            u"  to listen on. It can be also a host name that translates to a multicast\n"
            u"  address.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --buffer-size value\n"
            u"      Specify the UDP socket receive buffer size (socket option).\n"
            u"\n"
            u"  -d value\n"
            u"  --display-interval value\n"
            u"      Specify the interval in seconds between two displays of the evaluated\n"
            u"      real-time input bitrate. The default is to never display the bitrate.\n"
            u"      This option is ignored if --evaluation-interval is not specified.\n"
            u"\n"
            u"  -e value\n"
            u"  --evaluation-interval value\n"
            u"      Specify that the real-time input bitrate shall be evaluated on a regular\n"
            u"      basis. The value specifies the number of seconds between two evaluations.\n"
            u"      By default, the real-time input bitrate is never evaluated and the input\n"
            u"      bitrate is evaluated from the PCR in the input packets.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -l address\n"
            u"  --local-address address\n"
            u"      Specify the IP address of the local interface on which to listen.\n"
            u"      It can be also a host name that translates to a local address.\n"
            u"      By default, listen on all local interfaces.\n"
            u"\n"
            u"  -r\n"
            u"  --reuse-port\n"
            u"      Set the reuse port socket option.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPOutput::IPOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast.", u"[options] address:port"),
    _sock(false, *tsp_),
    _pkt_burst(DEF_PACKET_BURST)
{
    option(u"",               0,  STRING, 1, 1);
    option(u"local-address", 'l', STRING);
    option(u"packet-burst",  'p', INTEGER, 0, 1, 1, MAX_PACKET_BURST);
    option(u"ttl",           't', POSITIVE);

    setHelp(u"Parameter:\n"
            u"  The parameter address:port describes the destination for UDP packets.\n"
            u"  The 'address' specifies an IP address which can be either unicast or\n"
            u"  multicast. It can be also a host name that translates to an IP address.\n"
            u"  The 'port' specifies the destination UDP port.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -l address\n"
            u"  --local-address address\n"
            u"      When the destination is a multicast address, specify the IP address\n"
            u"      of the outgoing local interface. It can be also a host name that\n"
            u"      translates to a local address.\n"
            u"\n"
            u"  -p value\n"
            u"  --packet-burst value\n"
            u"      Specifies how many TS packets should be grouped into a UDP packet.\n"
            u"      The default is " TS_STRINGIFY(DEF_PACKET_BURST) u", the maximum is "
            TS_STRINGIFY(MAX_PACKET_BURST) u".\n"
            u"\n"
            u"  -t value\n"
            u"  --ttl value\n"
            u"      Specifies the TTL (Time-To-Live) socket option. The actual option\n"
            u"      is either \"Unicast TTL\" or \"Multicast TTL\", depending on the\n"
            u"      destination address. Remember that the default Multicast TTL is 1\n"
            u"      on most systems.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::IPInput::start()
{
    // Get command line arguments
    _eval_time = MilliSecPerSec * intValue<MilliSecond>(u"evaluation-interval", 0);
    _display_time = MilliSecPerSec * intValue<MilliSecond>(u"display-interval", 0);
    UString destination(value(u""));
    UString local(value(u"local-address"));
    size_t recv_bufsize = intValue<size_t>(u"buffer-size", 0);
    bool reuse_port = present(u"reuse-port");

    // Resolve specified destination address:port
    SocketAddress dest_addr;
    if (!dest_addr.resolve (destination, *tsp)) {
        return false;
    }

    // If a destination address is specified, it must be a multicast address
    if (dest_addr.hasAddress() && !dest_addr.isMulticast()) {
        tsp->error(u"address %s is not multicast", {dest_addr.toString()});
        return false;
    }

    // The destination port is mandatory
    if (!dest_addr.hasPort()) {
        tsp->error(u"no UDP port specified in %s", {destination});
        return false;
    }

    // Translate optional local address
    IPAddress local_ip;
    if (!local.empty() && !local_ip.resolve (local, *tsp)) {
        return false;
    }

    // The local socket address to bind is the optional local IP address and the destination port
    SocketAddress local_addr(local_ip, dest_addr.port());

    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Initialize socket.
    // Note: On Windows, bind must be done *before* joining multicast groups.
    bool ok =
        (!reuse_port || _sock.reusePort (true, *tsp)) &&
        (recv_bufsize <= 0 ||_sock.setReceiveBufferSize (recv_bufsize, *tsp)) &&
        _sock.bind (local_addr, *tsp) &&
        (!dest_addr.hasAddress() || _sock.addMembership (dest_addr, local_ip, *tsp));

    if (!ok) {
        _sock.close();
        return false;
    }

    // Socket now ready.
    // Initialize working data.
    _inbuf_count = _inbuf_next = 0;
    _start = _start_0 = _start_1 = _next_display = Time::Epoch;
    _packets = _packets_0 = _packets_1 = 0;

    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::IPInput::stop()
{
    _sock.close();
    return true;
}


//----------------------------------------------------------------------------
// Input destructor
//----------------------------------------------------------------------------

ts::IPInput::~IPInput()
{
    _sock.close();
}


//----------------------------------------------------------------------------
// Input bitrate evaluation method
//----------------------------------------------------------------------------

ts::BitRate ts::IPInput::getBitrate()
{
    if (_eval_time <= 0 || _start_0 == _start_1) {
        // Input bitrate not evaluated at all or first evaluation period not yet complete
        return 0;
    }
    else {
        // Evaluate bitrate since start of previous evaluation period.
        // The current period may be too short for correct evaluation.
        const MilliSecond ms = Time::CurrentUTC() - _start_0;
        return ms == 0 ? 0 : BitRate ((_packets_0 * PKT_SIZE * 8 * MilliSecPerSec) / ms);
    }
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::IPInput::receive (TSPacket* buffer, size_t max_packets)
{
    // Check if we receive new packets or process remain of previous buffer.
    bool new_packets = false;

    // If there is no remaining packet in the input buffer, wait for a UDP
    // message. Loop until we get some TS packets.
    while (_inbuf_count <= 0) {

        // Wait for a UDP message
        SocketAddress sender;
        SocketAddress destination;
        size_t insize;
        if (!_sock.receive (_inbuf, sizeof(_inbuf), insize, sender, destination, tsp, *tsp)) {
            return 0;
        }

        // Locate the TS packets inside the UDP message. Basically, we
        // expect the message to contain only TS packets. However, we
        // will face the following situations:
        // - Presence of a header preceeding the first TS packet (typically
        //   when the TS packets are encapsulated in RTP).
        // - Presence of a truncated packet at the end of message.

        // To face the first situation, we look backward from the end of
        // the message, looking for a 0x47 sync byte every 188 bytes, going
        // backward.

        const uint8_t* p;
        for (p = _inbuf + insize; p >= _inbuf + PKT_SIZE && p[-int(PKT_SIZE)] == SYNC_BYTE; p -= PKT_SIZE) {}

        if (p < _inbuf + insize) {
            // Some packets were found
            _inbuf_next = p - _inbuf;
            _inbuf_count = (_inbuf + insize - p) / PKT_SIZE;
            new_packets = true;
            break; // exit receive loop
        }

        // If no TS packet is found using the first method, we restart from
        // the beginning of the message, looking for a 0x47 sync byte every
        // 188 bytes, going forward. If we find this pattern, followed by
        // less than 188 bytes, then we have found a sequence of TS packets.

        const uint8_t* max = _inbuf + insize - PKT_SIZE; // max address for a TS packet
        _inbuf_count = 0;

        for (p = _inbuf; p <= max; p++) {
            if (*p == SYNC_BYTE) {
                // Verify that we get a 0x47 sync byte every 188 bytes up
                // to the end of message (not leaving more than one truncated
                // TS packet at the end of the message).
                const uint8_t* end;
                for (end = p; end <= max && *end == SYNC_BYTE; end += PKT_SIZE) {}
                if (end > max) {
                    // Less than 188 bytes after last packet. Consider we are OK
                    _inbuf_next = p - _inbuf;
                    _inbuf_count = (end - p) / PKT_SIZE;
                    new_packets = true;
                    break; // exit packet search loop
                }
            }
        }

        if (new_packets) {
            break; // exit receive loop
        }

        // No TS packet found in UDP message, wait for another one.
        tsp->debug(u"no TS packet in message from %s, %s bytes", {sender.toString(), insize});
    }

    // If new packets were received, we may need to re-evaluate the real-time input bitrate.
    if (new_packets && _eval_time > 0) {

        const Time now (Time::CurrentUTC());

        // Detect start time
        if (_packets == 0) {
            _start = _start_0 = _start_1 = now;
            if (_display_time > 0) {
                _next_display = now + _display_time;
            }
        }

        // Count packets
        _packets += _inbuf_count;
        _packets_0 += _inbuf_count;
        _packets_1 += _inbuf_count;

        // Detect new evaluation period
        if (now >= _start_1 + _eval_time) {
            _start_0 = _start_1;
            _packets_0 = _packets_1;
            _start_1 = now;
            _packets_1 = 0;

        }

        // Check if evaluated bitrate should be displayed
        if (_display_time > 0 && now >= _next_display) {
            _next_display += _display_time;
            const MilliSecond ms_current = Time::CurrentUTC() - _start_0;
            const MilliSecond ms_total = Time::CurrentUTC() - _start;
            const BitRate br_current = ms_current == 0 ? 0 : BitRate ((_packets_0 * PKT_SIZE * 8 * MilliSecPerSec) / ms_current);
            const BitRate br_average = ms_total == 0 ? 0 : BitRate ((_packets * PKT_SIZE * 8 * MilliSecPerSec) / ms_total);
            tsp->info(u"IP input bitrate: %s, average: %s", {
                br_current == 0 ? u"undefined" : UString::Decimal(br_current) + u" b/s",
                br_average == 0 ? u"undefined" : UString::Decimal(br_average) + u" b/s"});
        }
    }

    // Return packets from the input buffer
    size_t pkt_cnt = std::min (_inbuf_count, max_packets);
    ::memcpy (buffer, _inbuf + _inbuf_next, pkt_cnt * PKT_SIZE);
    _inbuf_count -= pkt_cnt;
    _inbuf_next += pkt_cnt * PKT_SIZE;

    return pkt_cnt;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::IPOutput::start()
{
    // Get command line arguments
    UString dest_name(value(u""));
    UString loc_name(value(u"local-address"));
    int ttl = intValue(u"ttl", 0);
    _pkt_burst = intValue(u"packet-burst", DEF_PACKET_BURST);

    // Create UDP socket
    bool ok = _sock.open (*tsp);

    if (ok) {
        ok = _sock.setDefaultDestination (dest_name, *tsp) &&
            (loc_name.empty() || _sock.setOutgoingMulticast (loc_name, *tsp)) &&
            (ttl <= 0 || _sock.setTTL (ttl, _sock.setTTL (ttl, *tsp)));
        if (!ok) {
            _sock.close();
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::IPOutput::stop()
{
    _sock.close();
    return true;
}


//----------------------------------------------------------------------------
// Output destructor
//----------------------------------------------------------------------------

ts::IPOutput::~IPOutput()
{
    _sock.close();
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::IPOutput::send (const TSPacket* pkt, size_t packet_count)
{
    // Send TS packets in UDP messages, grouped according to burst size.

    while (packet_count > 0) {
        size_t count = std::min (packet_count, _pkt_burst);
        if (!_sock.send (pkt, count * PKT_SIZE, *tsp)) {
            return false;
        }
        pkt += count;
        packet_count -= count;
    }

    return true;
}

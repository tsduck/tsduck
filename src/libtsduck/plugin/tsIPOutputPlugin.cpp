//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsIPOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsSystemRandomGenerator.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"ip", ts::IPOutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::IPOutputPlugin::REFERENCE = 0;

// Grouping TS packets in UDP packets

#define DEF_PACKET_BURST    7  // 1316 B, fits (with headers) in Ethernet MTU
#define MAX_PACKET_BURST  128  // ~ 48 kB


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPOutputPlugin::IPOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast", u"[options] address:port"),
    _destination(),
    _local_addr(),
    _local_port(SocketAddress::AnyPort),
    _ttl(0),
    _tos(-1),
    _pkt_burst(DEF_PACKET_BURST),
    _enforce_burst(false),
    _use_rtp(false),
    _rtp_pt(RTP_PT_MP2T),
    _rtp_fixed_sequence(false),
    _rtp_start_sequence(0),
    _rtp_sequence(0),
    _rtp_fixed_ssrc(false),
    _rtp_user_ssrc(0),
    _rtp_ssrc(0),
    _pcr_user_pid(PID_NULL),
    _pcr_pid(PID_NULL),
    _last_pcr(INVALID_PCR),
    _last_rtp_pcr(INVALID_PCR),
    _last_rtp_pcr_pkt(0),
    _rtp_pcr_offset(0),
    _pkt_count(0),
    _sock(false, *tsp_),
    _out_count(0),
    _out_buffer()
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"The parameter address:port describes the destination for UDP packets. "
         u"The 'address' specifies an IP address which can be either unicast or "
         u"multicast. It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination UDP port.");

    option(u"enforce-burst", 'e');
    help(u"enforce-burst",
         u"Enforce that the number of TS packets per UDP packet is exactly what is specified "
         u"in option --packet-burst. By default, this is only a maximum value.");

    option(u"local-address", 'l', STRING);
    help(u"local-address",
         u"When the destination is a multicast address, specify the IP address "
         u"of the outgoing local interface. It can be also a host name that "
         u"translates to a local address.");

    option(u"local-port", 0, UINT16);
    help(u"local-port",
         u"Specify the local UDP source port for outgoing packets. "
         u"By default, a random source port is used.");

    option(u"packet-burst", 'p', INTEGER, 0, 1, 1, MAX_PACKET_BURST);
    help(u"packet-burst",
         u"Specifies the maximum number of TS packets per UDP packet. "
         u"The default is " TS_STRINGIFY(DEF_PACKET_BURST) u", the maximum is " TS_STRINGIFY(MAX_PACKET_BURST) u".");

    option(u"tos", 's', INTEGER, 0, 1, 1, 255);
    help(u"tos",
         u"Specifies the TOS (Type-Of-Service) socket option. Setting this value "
         u"may depend on the user's privilege or operating system configuration.");

    option(u"ttl", 't', INTEGER, 0, 1, 1, 255);
    help(u"ttl",
         u"Specifies the TTL (Time-To-Live) socket option. The actual option "
         u"is either \"Unicast TTL\" or \"Multicast TTL\", depending on the "
         u"destination address. Remember that the default Multicast TTL is 1 "
         u"on most systems.");

    option(u"rtp", 'r');
    help(u"rtp",
         u"Use the Real-time Transport Protocol (RTP) in output UDP datagrams. "
         u"By default, TS packets are sent in UDP datagrams without encapsulation.");

    option(u"payload-type", 0, INTEGER, 0, 1, 0, 127);
    help(u"payload-type",
        u"With --rtp, specify the payload type. "
        u"By default, use " + UString::Decimal(RTP_PT_MP2T) + u", the standard RTP type for MPEG2-TS.");

    option(u"pcr-pid", 0, PIDVAL);
    help(u"pcr-pid",
        u"With --rtp, specify the PID containing the PCR's which are used as reference for RTP timestamps. "
        u"By default, use the first PID containing PCR's.");

    option(u"start-sequence-number", 0, UINT16);
    help(u"start-sequence-number",
        u"With --rtp, specify the initial sequence number. "
        u"By default, use a random value. Do not modify unless there is a good reason to do so.");

    option(u"ssrc-identifier", 0, UINT32);
    help(u"ssrc-identifier",
        u"With --rtp, specify the SSRC identifier. "
        u"By default, use a random value. Do not modify unless there is a good reason to do so.");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::getOptions()
{
    // Get command line arguments
    getValue(_destination, u"");
    getValue(_local_addr, u"local-address");
    _local_port = intValue<uint16_t>(u"local-port", SocketAddress::AnyPort);
    _ttl = intValue<int>(u"ttl", 0);
    _tos = intValue<int>(u"tos", -1);
    _pkt_burst = intValue<size_t>(u"packet-burst", DEF_PACKET_BURST);
    _enforce_burst = present(u"enforce-burst");
    _use_rtp = present(u"rtp");
    _rtp_pt = intValue<uint8_t>(u"payload-type", RTP_PT_MP2T);
    _rtp_fixed_sequence = present(u"start-sequence-number");
    _rtp_start_sequence = intValue<uint16_t>(u"start-sequence-number");
    _rtp_fixed_ssrc = present(u"ssrc-identifier");
    _rtp_user_ssrc = intValue<uint32_t>(u"ssrc-identifier");
    _pcr_user_pid = intValue<PID>(u"pcr-pid", PID_NULL);
    return true;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::start()
{
    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Configure socket.
    const SocketAddress local(IPAddress::AnyAddress, _local_port);
    if ((_local_port != SocketAddress::AnyPort && (!_sock.reusePort(true, *tsp) || !_sock.bind(local, *tsp))) ||
        !_sock.setDefaultDestination(_destination, *tsp) ||
        (!_local_addr.empty() && !_sock.setOutgoingMulticast(_local_addr, *tsp)) ||
        (_tos >= 0 && !_sock.setTOS(_tos, *tsp)) ||
        (_ttl > 0 && !_sock.setTTL(_ttl, *tsp)))
    {
        _sock.close(*tsp);
        return false;
    }

    // The output buffer is empty.
    if (_enforce_burst) {
        _out_buffer.resize(_pkt_burst);
        _out_count = 0;
    }

    // Initialize RTP parameters.
    if (_use_rtp) {
        // Use a system PRNG. This type of RNG does not need to be seeded.
        SystemRandomGenerator prng;
        if (_rtp_fixed_sequence) {
            _rtp_sequence = _rtp_start_sequence;
        }
        else if (!prng.readInt(_rtp_sequence)) {
            tsp->error(u"random number generation error");
            return false;
        }
        if (_rtp_fixed_ssrc) {
            _rtp_ssrc = _rtp_user_ssrc;
        }
        else if (!prng.readInt(_rtp_ssrc)) {
            tsp->error(u"random number generation error");
            return false;
        }
    }

    // Other states.
    _pcr_pid = _pcr_user_pid;
    _last_pcr = INVALID_PCR;
    _last_rtp_pcr = 0;  // Always start timestamps at zero
    _last_rtp_pcr_pkt = 0;
    _rtp_pcr_offset = 0;
    _pkt_count = 0;

    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::stop()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::send(const TSPacket* pkt, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Send TS packets in UDP messages, grouped according to burst size.
    // Minimum number of TS packets per UDP packet.
    assert(_pkt_burst > 0);
    const size_t min_burst = _enforce_burst ? _pkt_burst - 1 : 0;

    // First, with --enforce-burst, fill partial output buffer.
    if (_out_count > 0) {
        assert(_enforce_burst);
        assert(_out_count < _pkt_burst);

        // Copy as many packets as possible in output buffer.
        const size_t count = std::min(packet_count, _pkt_burst - _out_count);
        TSPacket::Copy(&_out_buffer[_out_count], pkt, count);
        pkt += count;
        packet_count -= count;
        _out_count += count;

        // Send the output buffer when full.
        if (_out_count == _pkt_burst) {
            if (!sendDatagram(_out_buffer.data(), _out_count)) {
                return false;
            }
            _out_count = 0;
        }
    }

    // Send subsequent packets from the global buffer.
    while (packet_count > min_burst) {
        size_t count = std::min(packet_count, _pkt_burst);
        if (!sendDatagram(pkt, count)) {
            return false;
        }
        pkt += count;
        packet_count -= count;
    }

    // If remaining packets are present, save them in output buffer.
    if (packet_count > 0) {
        assert(_enforce_burst);
        assert(_out_count == 0);
        assert(packet_count < _pkt_burst);
        TSPacket::Copy(_out_buffer.data(), pkt, packet_count);
        _out_count = packet_count;
    }
    return true;
}


//----------------------------------------------------------------------------
// Send contiguous packets in one single datagram.
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::sendDatagram(const TSPacket* pkt, size_t packet_count)
{
    bool status = true;

    if (_use_rtp) {
        // RTP datagram are relatively trivial to build, except the time stamp.
        // We cannot use the wall clock time because the plugin is likely to burst its output.
        // So, we try to synchronize RTP timestamps with PCR's from one PID.
        // But this is not trivial since the PCR may not be accurate or may loop back.
        // As long as the first PCR is not seen, increment timestamps from zero, using TS bitrate as reference.
        // At the first PCR, compute the difference between the current RTP timestamp and this PCR.
        // Then keep this difference and resynchronize at each PCR.
        // But never jump back in RTP timestamps, only increase "more slowly" when adjusting.

        // Build an RTP datagram. Use a simple RTP header without options nor extensions.
        ByteBlock buffer(RTP_HEADER_SIZE + packet_count * PKT_SIZE);

        // Build the RTP header, except the timestamp.
        buffer[0] = 0x80;             // Version = 2, P = 0, X = 0, CC = 0
        buffer[1] = _rtp_pt & 0x7F;   // M = 0, payload type
        PutUInt16(&buffer[2], _rtp_sequence++);
        PutUInt32(&buffer[8], _rtp_ssrc);

        // Get current bitrate to compute timestamps.
        const BitRate bitrate = tsp->bitrate();

        // Look for a PCR in one of the packets to send.
        // If found, we adjust this PCR for the first packet in the datagram.
        uint64_t pcr = INVALID_PCR;
        for (size_t i = 0; i < packet_count; i++) {
            const bool hasPCR = pkt[i].hasPCR();
            const PID pid = pkt[i].getPID();

            // Detect PCR PID if not yet known.
            if (hasPCR && _pcr_pid == PID_NULL) {
                _pcr_pid = pid;
            }

            // Detect PCR presence.
            if (hasPCR && pid == _pcr_pid) {
                pcr = pkt[i].getPCR();
                // If the bitrate is known and the packet containing the PCR is not the first one,
                // compute the theoretical timestamp of the first packet in the datagram.
                if (i > 0 && bitrate > 0) {
                    pcr -= (i * 8 * PKT_SIZE * uint64_t(SYSTEM_CLOCK_FREQ)) / bitrate;
                }
                break;
            }
        }

        // Extrapolate the RTP timestamp from the previous one, using current bitrate.
        // This value may be replaced if a valid PCR is present in this datagram.
        uint64_t rtp_pcr = _last_rtp_pcr;
        if (bitrate > 0) {
            rtp_pcr += ((_pkt_count - _last_rtp_pcr_pkt) * 8 * PKT_SIZE * uint64_t(SYSTEM_CLOCK_FREQ)) / bitrate;
        }

        // If the current datagram contains a PCR, recompute the RTP timestamp more precisely.
        if (pcr != INVALID_PCR) {
            if (_last_pcr == INVALID_PCR || pcr < _last_pcr) {
                // This is the first PCR in the stream or the PCR has jumped back in the past.
                // For this time only, we keep the extrapolated PCR.
                // Compute the difference between PCR and RTP timestamps.
                _rtp_pcr_offset = pcr - rtp_pcr;
                tsp->verbose(u"RTP timestamps resynchronized with PCR PID 0x%X (%d)", {_pcr_pid, _pcr_pid});
                tsp->debug(u"new PCR-RTP offset: %d", {_rtp_pcr_offset});
            }
            else {
                // PCR are normally increasing, drop extrapolated value, resynchronize with PCR.
                uint64_t adjusted_rtp_pcr = pcr - _rtp_pcr_offset;
                if (adjusted_rtp_pcr <= _last_rtp_pcr) {
                    // The adjustment would make the RTP timestamp go backward. We do not want that.
                    // We increase the RTP timestamp "more slowly", by 25% of the extrapolated value.
                    tsp->debug(u"RTP adjustment from PCR would step backward by %d", {((_last_rtp_pcr - adjusted_rtp_pcr) * RTP_RATE_MP2T) / SYSTEM_CLOCK_FREQ});
                    adjusted_rtp_pcr = _last_rtp_pcr + (rtp_pcr - _last_rtp_pcr) / 4;
                }
                rtp_pcr = adjusted_rtp_pcr;
            }

            // Keep last PCR value.
            _last_pcr = pcr;
        }

        // Insert the RTP timestamp in RTP clock units.
        PutUInt32(&buffer[4], uint32_t((rtp_pcr * RTP_RATE_MP2T) / SYSTEM_CLOCK_FREQ));

        // Remember position and value of last datagram.
        _last_rtp_pcr = rtp_pcr;
        _last_rtp_pcr_pkt = _pkt_count;

        // Copy the TS packets after the RTP header and send the packets.
        ::memcpy(buffer.data() + RTP_HEADER_SIZE, pkt, packet_count * PKT_SIZE);
        status = _sock.send(buffer.data(), buffer.size(), *tsp);
    }
    else {
        // No RTP, send TS packets directly as datagram.
        status = _sock.send(pkt, packet_count * PKT_SIZE, *tsp);
    }

    // Count packets datagram per datagram.
    _pkt_count += packet_count;

    return status;
}

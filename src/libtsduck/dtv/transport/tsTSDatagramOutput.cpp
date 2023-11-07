//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSDatagramOutput.h"
#include "tsSystemRandomGenerator.h"
#include "tsDuckContext.h"
#include "tsArgs.h"



//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSDatagramOutput::TSDatagramOutput(TSDatagramOutputOptions flags, TSDatagramOutputHandlerInterface* output) :
    _flags(flags),
    _output(output != nullptr ? output : this),
    _raw_udp(output == nullptr)
{
}


//----------------------------------------------------------------------------
// Add command line option definitions in an Args.
//----------------------------------------------------------------------------

void ts::TSDatagramOutput::defineArgs(Args& args)
{
    args.option(u"packet-burst", 'p', Args::INTEGER, 0, 1, 1, MAX_PACKET_BURST);
    args.help(u"packet-burst",
              u"Specifies the maximum number of TS packets per UDP packet. "
              u"The default is " + UString::Decimal(DEFAULT_PACKET_BURST) +
              u", the maximum is " + UString::Decimal(MAX_PACKET_BURST) + u".");

    // Enforcing burst can be hard-coded.
    if (!(_flags & TSDatagramOutputOptions::ALWAYS_BURST)) {
        args.option(u"enforce-burst", 'e');
        args.help(u"enforce-burst",
                  u"Enforce that the number of TS packets per UDP packet is exactly what is specified "
                  u"in option --packet-burst. By default, this is only a maximum value.");
    }

    // The following options are defined only when RTP is allowed.
    if ((_flags & TSDatagramOutputOptions::ALLOW_RTP) != TSDatagramOutputOptions::NONE) {
        args.option(u"rtp", 'r');
        args.help(u"rtp",
                  u"Use the Real-time Transport Protocol (RTP) in output UDP datagrams. "
                  u"By default, TS packets are sent in UDP datagrams without encapsulation.");

        args.option(u"payload-type", 0, Args::INTEGER, 0, 1, 0, 127);
        args.help(u"payload-type",
                  u"With --rtp, specify the payload type. "
                  u"By default, use " + UString::Decimal(RTP_PT_MP2T) + u", the standard RTP type for MPEG2-TS.");

        args.option(u"pcr-pid", 0, Args::PIDVAL);
        args.help(u"pcr-pid",
                  u"With --rtp, specify the PID containing the PCR's which are used as reference for RTP timestamps. "
                  u"By default, use the first PID containing PCR's.");

        args.option(u"start-sequence-number", 0, Args::UINT16);
        args.help(u"start-sequence-number",
                  u"With --rtp, specify the initial sequence number. "
                  u"By default, use a random value. Do not modify unless there is a good reason to do so.");

        args.option(u"ssrc-identifier", 0, Args::UINT32);
        args.help(u"ssrc-identifier",
                  u"With --rtp, specify the SSRC identifier. "
                  u"By default, use a random value. Do not modify unless there is a good reason to do so.");
    }

    // The following options are defined only when raw UDP is allowed.
    if (_raw_udp) {
        args.option(u"", 0, Args::IPSOCKADDR, 1, 1);
        args.help(u"",
                  u"The parameter address:port describes the destination for UDP packets. "
                  u"The 'address' specifies an IP address which can be either unicast or "
                  u"multicast. It can be also a host name that translates to an IP address. "
                  u"The 'port' specifies the destination UDP port.");

        args.option(u"buffer-size", 'b', Args::UNSIGNED);
        args.help(u"buffer-size", u"Specify the UDP socket send buffer size in bytes (socket option).");

        args.option(u"disable-multicast-loop", 'd');
        args.help(u"disable-multicast-loop",
                  u"Disable multicast loopback. By default, outgoing multicast packets are looped back on local interfaces, "
                  u"if an application added membership on the same multicast group. This option disables this.\n"
                  u"Warning: On output sockets, this option is effective only on Unix systems (Linux, macOS, BSD). "
                  u"On Windows systems, this option applies only to input sockets.");

        args.option(u"force-local-multicast-outgoing", 'f');
        args.help(u"force-local-multicast-outgoing",
                  u"When the destination is a multicast address and --local-address is specified, "
                  u"force multicast outgoing traffic on this local interface (socket option IP_MULTICAST_IF). "
                  u"Use this option with care. Its usage depends on the operating system. "
                  u"If no route is declared for this destination address, this option may be necessary "
                  u"to force the multicast to the specified local interface. On the other hand, if a route is "
                  u"declared, this option may transport multicast IP packets in unicast Ethernet frames "
                  u"to the gateway, preventing multicast reception on the local network (seen on Linux).");

        args.option(u"local-address", 'l', Args::IPADDR);
        args.help(u"local-address",
                  u"When the destination is a multicast address, specify the IP address "
                  u"of the outgoing local interface. It can be also a host name that "
                  u"translates to a local address.");

        args.option(u"local-port", 0, Args::UINT16);
        args.help(u"local-port",
                  u"Specify the local UDP source port for outgoing packets. "
                  u"By default, a random source port is used.");

        args.option(u"rs204");
        args.help(u"rs204",
                  u"Use 204-byte format for TS packets in UDP datagrams. "
                  u"Each TS packet is followed by a zeroed placeholder for a 16-byte Reed-Solomon trailer.");

        args.option(u"tos", 's', Args::INTEGER, 0, 1, 1, 255);
        args.help(u"tos",
                  u"Specifies the TOS (Type-Of-Service) socket option. Setting this value "
                  u"may depend on the user's privilege or operating system configuration.");

        args.option(u"ttl", 't', Args::INTEGER, 0, 1, 1, 255);
        args.help(u"ttl",
                  u"Specifies the TTL (Time-To-Live) socket option. The actual option "
                  u"is either \"Unicast TTL\" or \"Multicast TTL\", depending on the "
                  u"destination address. Remember that the default Multicast TTL is 1 "
                  u"on most systems.");
    }
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSDatagramOutput::loadArgs(DuckContext& duck, Args& args)
{
    args.getIntValue(_pkt_burst, u"packet-burst", DEFAULT_PACKET_BURST);
    _enforce_burst = (_flags & TSDatagramOutputOptions::ALWAYS_BURST) != TSDatagramOutputOptions::NONE || args.present(u"enforce-burst");

    if ((_flags & TSDatagramOutputOptions::ALLOW_RTP) != TSDatagramOutputOptions::NONE) {
        _use_rtp = args.present(u"rtp");
        args.getIntValue(_rtp_pt, u"payload-type", RTP_PT_MP2T);
        _rtp_fixed_sequence = args.present(u"start-sequence-number");
        args.getIntValue(_rtp_start_sequence, u"start-sequence-number");
        _rtp_fixed_ssrc = args.present(u"ssrc-identifier");
        args.getIntValue(_rtp_user_ssrc, u"ssrc-identifier");
        args.getIntValue(_pcr_user_pid, u"pcr-pid", PID_NULL);
    }

    if (_raw_udp) {
        args.getSocketValue(_destination, u"");
        args.getIPValue(_local_addr, u"local-address");
        args.getIntValue(_local_port, u"local-port", IPv4SocketAddress::AnyPort);
        args.getIntValue(_ttl, u"ttl", 0);
        args.getIntValue(_tos, u"tos", -1);
        args.getIntValue(_send_bufsize, u"buffer-size", 0);
        _mc_loopback = !args.present(u"disable-multicast-loop");
        _force_mc_local = args.present(u"force-local-multicast-outgoing");
        _rs204_format = args.present(u"rs204");
    }

    return true;
}


//----------------------------------------------------------------------------
// Open and initialize the TS packet output.
//----------------------------------------------------------------------------

bool ts::TSDatagramOutput::open(Report& report)
{
    if (_is_open) {
        report.error(u"TSDatagramOutput is already open");
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
            report.error(u"random number generation error");
            return false;
        }
        if (_rtp_fixed_ssrc) {
            _rtp_ssrc = _rtp_user_ssrc;
        }
        else if (!prng.readInt(_rtp_ssrc)) {
            report.error(u"random number generation error");
            return false;
        }
    }

    // Initialize raw UDP socket
    if (_raw_udp) {
        if (!_sock.open(report)) {
            return false;
        }
        const IPv4SocketAddress local(_local_addr, _local_port);
        if ((_local_port != IPv4SocketAddress::AnyPort && !_sock.reusePort(true, report)) ||
            !_sock.bind(local, report) ||
            !_sock.setDefaultDestination(_destination, report) ||
            !_sock.setMulticastLoop(_mc_loopback, report) ||
            (_force_mc_local && _destination.isMulticast() && _local_addr.hasAddress() && !_sock.setOutgoingMulticast(_local_addr, report)) ||
            (_send_bufsize > 0 && !_sock.setSendBufferSize(_send_bufsize, report)) ||
            (_tos >= 0 && !_sock.setTOS(_tos, report)) ||
            (_ttl > 0 && !_sock.setTTL(_ttl, report)))
        {
            _sock.close(report);
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

    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Close the TS packet output.
//----------------------------------------------------------------------------

bool ts::TSDatagramOutput::close(const BitRate& bitrate, Report& report)
{
    bool success = true;
    if (_is_open) {
        // Flush incomplete datagram, if any.
        if (_out_count > 0) {
            success = sendPackets(_out_buffer.data(), _out_count, bitrate, report);
            _out_count = 0;
        }
        if (_raw_udp) {
            _sock.close(report);
        }
        _is_open = false;
    }
    return success;
}


//----------------------------------------------------------------------------
// Send TS packets.
//----------------------------------------------------------------------------

bool ts::TSDatagramOutput::send(const TSPacket* pkt, size_t packet_count, const BitRate& bitrate, Report& report)
{
    if (!_is_open) {
        report.error(u"TSDatagramOutput is not open");
        return false;
    }

    // Send TS packets in UDP messages, grouped according to burst size.
    // Minimum number of TS packets per UDP packet.
    assert(_pkt_burst > 0);
    const size_t min_burst = _enforce_burst ? _pkt_burst : 1;

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
            if (!sendPackets(_out_buffer.data(), _out_count, bitrate, report)) {
                return false;
            }
            _out_count = 0;
        }
    }

    // Send subsequent packets from the global buffer.
    while (packet_count >= min_burst) {
        size_t count = std::min(packet_count, _pkt_burst);
        if (!sendPackets(pkt, count, bitrate, report)) {
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

bool ts::TSDatagramOutput::sendPackets(const TSPacket* pkt, size_t packet_count, const BitRate& bitrate, Report& report)
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
        ByteBlock buffer(RTP_HEADER_SIZE + packet_count * PKT_RS_SIZE);

        // Build the RTP header, except the timestamp.
        buffer[0] = 0x80;             // Version = 2, P = 0, X = 0, CC = 0
        buffer[1] = _rtp_pt & 0x7F;   // M = 0, payload type
        PutUInt16(&buffer[2], _rtp_sequence++);
        PutUInt32(&buffer[8], _rtp_ssrc);

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
                    pcr -= ((i * PKT_SIZE_BITS * uint64_t(SYSTEM_CLOCK_FREQ)) / bitrate).toInt();
                }
                break;
            }
        }

        // Extrapolate the RTP timestamp from the previous one, using current bitrate.
        // This value may be replaced if a valid PCR is present in this datagram.
        uint64_t rtp_pcr = _last_rtp_pcr;
        if (bitrate > 0) {
            rtp_pcr += (((_pkt_count - _last_rtp_pcr_pkt) * PKT_SIZE_BITS * uint64_t(SYSTEM_CLOCK_FREQ)) / bitrate).toInt();
        }

        // If the current datagram contains a PCR, recompute the RTP timestamp more precisely.
        if (pcr != INVALID_PCR) {
            if (_last_pcr == INVALID_PCR || pcr < _last_pcr) {
                // This is the first PCR in the stream or the PCR has jumped back in the past.
                // For this time only, we keep the extrapolated PCR.
                // Compute the difference between PCR and RTP timestamps.
                _rtp_pcr_offset = pcr - rtp_pcr;
                report.verbose(u"RTP timestamps resynchronized with PCR PID 0x%X (%d)", {_pcr_pid, _pcr_pid});
                report.debug(u"new PCR-RTP offset: %d", {_rtp_pcr_offset});
            }
            else {
                // PCR are normally increasing, drop extrapolated value, resynchronize with PCR.
                uint64_t adjusted_rtp_pcr = pcr - _rtp_pcr_offset;
                if (adjusted_rtp_pcr <= _last_rtp_pcr) {
                    // The adjustment would make the RTP timestamp go backward. We do not want that.
                    // We increase the RTP timestamp "more slowly", by 25% of the extrapolated value.
                    report.debug(u"RTP adjustment from PCR would step backward by %d", {((_last_rtp_pcr - adjusted_rtp_pcr) * RTP_RATE_MP2T) / SYSTEM_CLOCK_FREQ});
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
        uint8_t* buf = buffer.data() + RTP_HEADER_SIZE;
        if (_rs204_format) {
            // Copy TS packets one by one with RS204 zero trailer. Since the default initial value
            // of the buffer vector is zero, there is no need to explicitly set the trailers.
            for (size_t i = 0; i < packet_count; ++i) {
                std::memcpy(buf, pkt++, PKT_SIZE);
                buf += PKT_SIZE + RS_SIZE;
            }
        }
        else {
            // Directly copy the TS packets and shrink the buffer (no RS204 trailers).
            std::memcpy(buf, pkt, packet_count * PKT_SIZE);
            buffer.resize(RTP_HEADER_SIZE + packet_count * PKT_SIZE);
        }
        status = _output->sendDatagram(buffer.data(), buffer.size(), report);
    }
    else if (_rs204_format) {
        // No RTP header, add TS trailer after each packet. Since the default initial value
        // of the buffer vector is zero, there is no need to explicitly set the trailers.
        ByteBlock buffer(packet_count * PKT_RS_SIZE);
        uint8_t* buf = buffer.data();
        for (size_t i = 0; i < packet_count; ++i) {
            std::memcpy(buf, pkt++, PKT_SIZE);
            buf += PKT_SIZE + RS_SIZE;
        }
        status = _output->sendDatagram(buffer.data(), buffer.size(), report);
    }
    else {
        // No RTP, send TS packets directly as datagram.
        status = _output->sendDatagram(pkt, packet_count * PKT_SIZE, report);
    }

    // Count packets datagram per datagram.
    _pkt_count += packet_count;

    return status;
}


//----------------------------------------------------------------------------
// Implementation of TSDatagramOutputHandlerInterface.
// The object is its own handler in case of raw UDP output.
//----------------------------------------------------------------------------

bool ts::TSDatagramOutput::sendDatagram(const void* address, size_t size, Report& report)
{
    return _sock.send(address, size, report);
}

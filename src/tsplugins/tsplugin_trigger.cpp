//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Trigger actions on selected labeled TS packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsForkPipe.h"
#include "tsByteBlock.h"
#include "tsUDPSocket.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TriggerPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(TriggerPlugin);
    public:
        // Implementation of plugin API
        TriggerPlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        PacketCounter    _minInterPacket; // Minimum interval in packets between two actions.
        MilliSecond      _minInterTime;   // Minimum interval in milliseconds between two actions.
        UString          _execute;        // Command to execute on trigger.
        UString          _udpDestination; // UDP/IP destination address:port.
        UString          _udpLocal;       // Name of outgoing local address (empty if unspecified).
        ByteBlock        _udpMessage;     // What to send as UDP message.
        int              _udpTTL;         // Time-to-live socket option.
        bool             _onStart;        // Trigger action on start.
        bool             _onStop;         // Trigger action on stop.
        bool             _allPackets;     // Trigger on all packets in the stream.
        bool             _allLabels;      // Need all labels to be set.
        TSPacketLabelSet _labels;         // Trigger on packets with these labels.

        // Working data:
        PacketCounter _lastPacket;    // Last action packet.
        Time          _lastTime;      // UTC time of last action.
        UDPSocket     _sock;          // Output socket.

        // Trigger the actions (exec, UDP).
        void trigger();
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"trigger", ts::TriggerPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TriggerPlugin::TriggerPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Trigger actions on selected TS packets", u"[options]"),
    _minInterPacket(0),
    _minInterTime(0),
    _execute(),
    _udpDestination(),
    _udpLocal(),
    _udpMessage(),
    _udpTTL(0),
    _onStart(false),
    _onStop(false),
    _allPackets(false),
    _allLabels(false),
    _labels(),
    _lastPacket(INVALID_PACKET_COUNTER),
    _lastTime(),
    _sock(false, *tsp)
{
    option(u"all-labels", 'a');
    help(u"all-labels",
         u"All labels from options --label shall be set on a packet to be selected (logical 'and'). "
         u"By default, a packet is selected if any label is set (logical 'or').");

    option(u"execute", 'e', STRING);
    help(u"execute", u"'command'",
         u"Run the specified command when the current packet triggers the actions.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"label", u"label1[-label2]",
         u"Trigger the actions on packets with any of the specified labels. "
         u"Labels should have typically be set by a previous plugin in the chain. "
         u"By default, without option --label, the actions are triggered on all packets in the stream. "
         u"Several --label options may be specified.\n\n"
         u"Note that the option --label is different from the generic option --only-label. "
         u"The generic option --only-label acts at tsp level and controls which packets are "
         u"passed to the plugin. All other packets are directly passed to the next plugin "
         u"without going through this plugin. The option --label, on the other hand, "
         u"is specific to the trigger plugin and selects packets with specific labels "
         u"among the packets which are passed to this plugin.");

    option(u"min-inter-packet", 0, UNSIGNED);
    help(u"min-inter-packet", u"count",
         u"Specify the minimum number of packets between two triggered actions. "
         u"Actions which should be triggered in the meantime are ignored.");

    option(u"min-inter-time", 0, UNSIGNED);
    help(u"min-inter-time", u"milliseconds",
         u"Specify the minimum time, in milliseconds, between two triggered actions. "
         u"Actions which should be triggered in the meantime are ignored.");

    option(u"udp", 'u', STRING);
    help(u"udp", u"address:port",
         u"Send a UDP/IP message to the specified destination when the current packet triggers the actions. "
         u"The 'address' specifies an IP address which can be either unicast or multicast. "
         u"It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination UDP port.");

    option(u"udp-message", 0, STRING);
    help(u"udp-message", u"hexa-string",
         u"With --udp, specifies the binary message to send as UDP datagram. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes.");

    option(u"local-address", 0, STRING);
    help(u"local-address", u"address",
         u"With --udp, when the destination is a multicast address, specify "
         u"the IP address of the outgoing local interface. It can be also a host "
         u"name that translates to a local address.");

    option(u"start");
    help(u"start", u"Trigger the actions on tsp start.");

    option(u"stop");
    help(u"stop", u"Trigger the actions on tsp stop.");

    option(u"ttl", 0, POSITIVE);
    help(u"ttl",
         u"With --udp, specifies the TTL (Time-To-Live) socket option. "
         u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
         u"depending on the destination address. Remember that the default "
         u"Multicast TTL is 1 on most systems.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::TriggerPlugin::getOptions()
{
    getIntValue(_minInterTime, u"min-inter-time");
    getIntValue(_minInterPacket, u"min-inter-packet");
    getValue(_execute, u"execute");
    getValue(_udpDestination, u"udp");
    getValue(_udpLocal, u"local-address");
    getIntValue(_udpTTL, u"ttl");
    getIntValues(_labels, u"label");
    _onStart = present(u"start");
    _onStop = present(u"stop");
    _allLabels = present(u"all-labels");
    _allPackets = !_onStart && !_onStop && _labels.none();

    if (present(u"udp-message") && !value(u"udp-message").hexaDecode(_udpMessage)) {
        tsp->error(u"invalid hexadecimal UDP message");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::TriggerPlugin::start()
{
    _lastPacket = INVALID_PACKET_COUNTER;
    _lastTime = Time::Epoch;

    // Initialize UDP output.
    if (!_udpDestination.empty()) {
        if (!_sock.open(*tsp)) {
            return false;
        }
        if (!_sock.setDefaultDestination(_udpDestination, *tsp) ||
            (!_udpLocal.empty() && !_sock.setOutgoingMulticast(_udpLocal, *tsp)) ||
            (_udpTTL > 0 && !_sock.setTTL(_udpTTL, *tsp)))
        {
            _sock.close(*tsp);
            return false;
        }
    }

    // Initial trigger.
    if (_onStart) {
        trigger();
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::TriggerPlugin::stop()
{
    // Final trigger.
    if (_onStop) {
        trigger();
    }

    if (_sock.isOpen()) {
        _sock.close(*tsp);
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TriggerPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Check if the packet shall be selected.
    Time now(Time::Epoch);
    const bool select =
        (_allPackets || (_allLabels && pkt_data.hasAllLabels(_labels)) || (!_allLabels && pkt_data.hasAnyLabel(_labels))) &&
        (_minInterPacket == 0 || _lastPacket == INVALID_PACKET_COUNTER || tsp->pluginPackets() >= _lastPacket + _minInterPacket) &&
        (_minInterTime == 0 || _lastTime == Time::Epoch || (now = Time::CurrentUTC()) >= _lastTime + _minInterTime);

    if (select) {
        // The packet shall be selected.
        tsp->debug(u"triggering action, packet %'d", {tsp->pluginPackets()});
        _lastTime = now == Time::Epoch ? Time::CurrentUTC() : now;
        _lastPacket = tsp->pluginPackets();
        trigger();
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Trigger the actions (exec, UDP).
//----------------------------------------------------------------------------

void ts::TriggerPlugin::trigger()
{
    // Execute external command.
    if (!_execute.empty()) {
        ForkPipe::Launch(_execute, *tsp, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
    }

    // Send message over a socket.
    if (_sock.isOpen()) {
        _sock.send(_udpMessage.data(), _udpMessage.size(), *tsp);
    }
}

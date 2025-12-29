//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TriggerPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TriggerPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        PacketCounter      _minInterPacket = 0;  // Minimum interval in packets between two actions.
        cn::milliseconds   _minInterTime {};     // Minimum interval in milliseconds between two actions.
        UString            _execute {};          // Command to execute on trigger.
        fs::path           _copy_source {};      // Copy that file ...
        fs::path           _copy_dest {};        // ... into this destination.
        IPSocketAddress    _udpDestination {};   // UDP/IP destination address:port.
        IPAddress          _udpLocal {};         // Name of outgoing local address (empty if unspecified).
        ByteBlock          _udpMessage {};       // What to send as UDP message.
        int                _udpTTL = 0;          // Time-to-live socket option.
        bool               _onStart = false;     // Trigger action on start.
        bool               _onStop = false;      // Trigger action on stop.
        bool               _allPackets = false;  // Trigger on all packets in the stream.
        bool               _allLabels = false;   // Need all labels to be set.
        bool               _once = false;        // Trigger the actions only once per label.
        TSPacketLabelSet   _labels {};           // Trigger on packets with these labels, from options.
        ForkPipe::WaitMode _wait_mode = ForkPipe::ASYNCHRONOUS;  // How to run executed commands.

        // Working data:
        PacketCounter    _lastPacket = INVALID_PACKET_COUNTER; // Last action packet.
        Time             _lastTime {};         // UTC time of last action.
        UDPSocket        _sock {false, IP::Any, *this}; // Output socket.
        TSPacketLabelSet _currentLabels {};    // Trigger on packets with these labels, during processing.

        // Trigger the actions (exec, UDP).
        void trigger();
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"trigger", ts::TriggerPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TriggerPlugin::TriggerPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Trigger actions on selected TS packets", u"[options]")
{
    option(u"all-labels", 'a');
    help(u"all-labels",
         u"All labels from options --label shall be set on a packet to be selected (logical 'and'). "
         u"By default, a packet is selected if any label is set (logical 'or').");

    option(u"copy", 'c', FILENAME);
    help(u"copy",
         u"Copy the specified file when the current packet triggers the actions.\n"
         u"See also option --destination.");

    option(u"destination", 'd', FILENAME);
    help(u"destination",
         u"With --copy, the file is copied to that specified destination. "
         u"If the specified path is an existing directory, the file is copied in that directory, with the same name as input.");

    option(u"execute", 'e', STRING);
    help(u"execute", u"'command'",
         u"Run the specified command when the current packet triggers the actions.\n"
         u"See also option --synchronous.");

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

    option<cn::milliseconds>(u"min-inter-time");
    help(u"min-inter-time",
         u"Specify the minimum time, in milliseconds, between two triggered actions. "
         u"Actions which should be triggered in the meantime are ignored.");

    option(u"udp", 'u', IPSOCKADDR);
    help(u"udp",
         u"Send a UDP/IP message to the specified destination when the current packet triggers the actions. "
         u"The 'address' specifies an IP address which can be either unicast or multicast. "
         u"It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination UDP port.");

    option(u"udp-message", 0, HEXADATA);
    help(u"udp-message",
         u"With --udp, specifies the binary message to send as UDP datagram. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes.");

    option(u"local-address", 0, IPADDR);
    help(u"local-address",
         u"With --udp, when the destination is a multicast address, specify "
         u"the IP address of the outgoing local interface. It can be also a host "
         u"name that translates to a local address.");

    option(u"once");
    help(u"once",
         u"Trigger the actions only once per label. "
         u"When a packet with one or more labels from option --label has triggered the actions, these labels are disabled.");

    option(u"start");
    help(u"start", u"Trigger the actions on tsp start.");

    option(u"stop");
    help(u"stop", u"Trigger the actions on tsp stop.");

    option(u"synchronous", 's');
    help(u"synchronous",
         u"With --execute, wait for the command to complete before processing the next packet. "
         u"By default, the command runs asynchronously.");

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
    getChronoValue(_minInterTime, u"min-inter-time");
    getIntValue(_minInterPacket, u"min-inter-packet");
    getValue(_execute, u"execute");
    getPathValue(_copy_source, u"copy");
    getPathValue(_copy_dest, u"destination");
    getSocketValue(_udpDestination, u"udp");
    getIPValue(_udpLocal, u"local-address");
    getIntValue(_udpTTL, u"ttl");
    getIntValues(_labels, u"label");
    getHexaValue(_udpMessage, u"udp-message");
    _onStart = present(u"start");
    _onStop = present(u"stop");
    _once = present(u"once");
    _allLabels = present(u"all-labels");
    _allPackets = !_onStart && !_onStop && _labels.none();
    _wait_mode = present(u"synchronous") ? ForkPipe::SYNCHRONOUS : ForkPipe::ASYNCHRONOUS;

    if (!_copy_source.empty() && _copy_dest.empty()) {
        error(u"--destination is required with --copy");
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
    _currentLabels = _labels;

    // Initialize UDP output.
    if (_udpDestination.hasAddress()) {
        if (!_sock.open(_udpDestination.generation(), *this)) {
            return false;
        }
        if (!_sock.setDefaultDestination(_udpDestination, *this) ||
            (!_udpLocal.hasAddress() && !_sock.setOutgoingMulticast(_udpLocal, *this)) ||
            (_udpTTL > 0 && !_sock.setTTL(_udpTTL, *this)))
        {
            _sock.close(*this);
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
        _sock.close(*this);
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
        (_allPackets || (_allLabels && pkt_data.hasAllLabels(_currentLabels)) || (!_allLabels && pkt_data.hasAnyLabel(_currentLabels))) &&
        (_minInterPacket == 0 || _lastPacket == INVALID_PACKET_COUNTER || tsp->pluginPackets() >= _lastPacket + _minInterPacket) &&
        (_minInterTime == cn::milliseconds::zero() || _lastTime == Time::Epoch || (now = Time::CurrentUTC()) >= _lastTime + _minInterTime);

    if (select) {
        // The packet shall be selected.
        debug(u"triggering action, packet %'d", tsp->pluginPackets());
        _lastTime = now == Time::Epoch ? Time::CurrentUTC() : now;
        _lastPacket = tsp->pluginPackets();
        trigger();

        // Reset the labels after first trigger.
        if (_once) {
            _currentLabels &= ~pkt_data.labels();
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Trigger the actions (exec, UDP).
//----------------------------------------------------------------------------

void ts::TriggerPlugin::trigger()
{
    // Copy user-specified file.
    if (!_copy_source.empty()) {
        fs::copy(_copy_source, _copy_dest, fs::copy_options::overwrite_existing, &ErrCodeReport(*this, u"error copying", _copy_source));
    }

    // Execute external command.
    if (!_execute.empty()) {
        ForkPipe::Launch(_execute, *this, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE, _wait_mode);
    }

    // Send message over a socket.
    if (_sock.isOpen()) {
        _sock.send(_udpMessage.data(), _udpMessage.size(), *this);
    }
}

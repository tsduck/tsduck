//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2025, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSRTOutputPlugin.h"
#include "tsPluginRepository.h"

#if !defined(TS_NO_SRT)
TS_REGISTER_OUTPUT_PLUGIN(u"srt", ts::SRTOutputPlugin);
#endif


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::SRTOutputPlugin::SRTOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Secure Reliable Transport (SRT)", u"[options] [address:port]")
{
    _datagram.defineArgs(*this);
    _sock.defineArgs(*this);

    option(u"multiple", 'm');
    help(u"multiple", u"When the receiver peer disconnects, wait for another one and continue.");

    option<cn::milliseconds>(u"restart-delay");
    help(u"restart-delay", u"With --multiple, wait the specified delay before restarting.");

    // These options are legacy, now use --listener and/or --caller.
    option(u"", 0, IPSOCKADDR_OA, 0, 1);
    help(u"" , u"Local [address:]port. This is a legacy parameter, now use --listener.");

    option(u"rendezvous", 0, IPSOCKADDR);
    help(u"rendezvous", u"address:port", u"Remote address and port. This is a legacy option, now use --caller.");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::getOptions()
{
    IPSocketAddress listener;
    IPSocketAddress rendezvous;
    getSocketValue(listener, u"");
    getSocketValue(rendezvous, u"rendezvous");
    _multiple = present(u"multiple");
    getChronoValue(_restart_delay, u"restart-delay");

    return _sock.setAddresses(listener, rendezvous, IPAddress(), *this) &&
           _sock.loadArgs(duck, *this) &&
           _datagram.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::start()
{
    bool success = _datagram.open(*this);
    IPSocketAddress local, remote;
    if (success) {
        success = _sock.open(_datagram.maxPayloadSize(), *this);
        if (!success) {
            _datagram.close(0, true, *this);
        }
        else if (_sock.getPeers(local, remote, *this)) {
            verbose(u"connected to %s (local: %s)", remote, local);
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::stop()
{
    _datagram.close(tsp->bitrate(), false, *this);
    _sock.close(*this);
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    return _datagram.send(packets, metadata, packet_count, tsp->bitrate(), *this);
}


//----------------------------------------------------------------------------
// Implementation of TSDatagramOutputHandlerInterface: send one datagram.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::sendDatagram(const void* address, size_t size, Report& report)
{
    // Loop on restart with multiple sessions.
    for (;;) {
        // Send the datagram.
        if (_sock.send(address, size, report)) {
            return true;
        }
        // Send error.
        if (!_sock.peerDisconnected()) {
            // Actual error, not a clean disconnection from the receiver, do not retry, even with --multiple.
            return false;
        }
        report.verbose(u"receiver disconnected%s", _multiple ? u", waiting for another one" : u"");
        if (!_multiple) {
            // No multiple sessions, terminate here.
            return false;
        }
        // Multiple sessions, close socket and re-open to acquire another receiver.
        _datagram.close(tsp->bitrate(), true, *this);
        _sock.close(*this);
        if (_restart_delay > cn::milliseconds::zero()) {
            std::this_thread::sleep_for(_restart_delay);
        }
        if (!start()) {
            return false;
        }
    }
}

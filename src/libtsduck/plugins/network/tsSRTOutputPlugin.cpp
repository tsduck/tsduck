//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2026, Lola Delannoy
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
    OutputPlugin(tsp_, u"Send TS packets using Secure Reliable Transport (SRT)", u"[options] [URL]")
{
    _datagram.defineArgs(*this);
    _sock.args().defineArgs(*this);

    option(u"multiple", 'm');
    help(u"multiple", u"When the receiver peer disconnects, wait for another one and continue.");

    option<cn::milliseconds>(u"restart-delay");
    help(u"restart-delay", u"With --multiple, wait the specified delay before restarting.");
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
    _sock.args().reset();
    _sock.args().sender = true; // output plugin -> sender

    _multiple = present(u"multiple");
    getChronoValue(_restart_delay, u"restart-delay");

    return _sock.args().loadArgs(duck, *this) && _datagram.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::start()
{
    bool success = _datagram.open();
    IPSocketAddress local, remote;
    if (success) {
        // Override SRT payload size if not set on the command line.
        if (_sock.args().payload_size < 0) {
            _sock.args().payload_size = int32_t(_datagram.maxPayloadSize());
        }
        // Open the SRT layer.
        success = _sock.open();
        if (!success) {
            _datagram.close(0, true);
        }
        else if (_sock.getPeers(local, remote)) {
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
    _datagram.close(tsp->bitrate(), false);
    _sock.close();
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    return _datagram.send(packets, metadata, packet_count, tsp->bitrate());
}


//----------------------------------------------------------------------------
// Implementation of TSDatagramOutputHandlerInterface: send one datagram.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::sendDatagram(const void* address, size_t size)
{
    // Loop on restart with multiple sessions.
    for (;;) {
        // Send the datagram.
        if (_sock.send(address, size)) {
            return true;
        }
        // Send error.
        if (!_sock.peerDisconnected()) {
            // Actual error, not a clean disconnection from the receiver, do not retry, even with --multiple.
            return false;
        }
        verbose(u"receiver disconnected%s", _multiple ? u", waiting for another one" : u"");
        if (!_multiple) {
            // No multiple sessions, terminate here.
            return false;
        }
        // Multiple sessions, close socket and re-open to acquire another receiver.
        _datagram.close(tsp->bitrate(), true);
        _sock.close();
        if (_restart_delay > cn::milliseconds::zero()) {
            std::this_thread::sleep_for(_restart_delay);
        }
        if (!start()) {
            return false;
        }
    }
}

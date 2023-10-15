//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Anthony Delannoy
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
    OutputPlugin(tsp_, u"Send TS packets using Secure Reliable Transport (SRT)", u"[options] [address:port]"),
    _datagram(TSDatagramOutputOptions::NONE, this)
{
    _datagram.defineArgs(*this);
    _sock.defineArgs(*this);

    option(u"multiple", 'm');
    help(u"multiple",
         u"When the receiver peer disconnects, wait for another one and continue.");

    option(u"restart-delay", 0, UNSIGNED);
    help(u"restart-delay", u"milliseconds",
         u"With --multiple, wait the specified number of milliseconds before restarting.");

    // These options are legacy, now use --listener and/or --caller.
    option(u"", 0, STRING, 0, 1);
    help(u"" , u"Local [address:]port. This is a legacy parameter, now use --listener.");

    option(u"rendezvous", 0, STRING);
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
    _multiple = present(u"multiple");
    getIntValue(_restart_delay, u"restart-delay", 0);
    return _sock.setAddresses(value(u""), value(u"rendezvous"), UString(), *tsp) &&
           _sock.loadArgs(duck, *this) &&
           _datagram.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::start()
{
    bool success = _datagram.open(*tsp);
    if (success) {
        success = _sock.open(*tsp);
        if (!success) {
            _datagram.close(0, *tsp);
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::stop()
{
    _datagram.close(tsp->bitrate(), *tsp);
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    return _datagram.send(packets, packet_count, tsp->bitrate(), *tsp);
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
        report.verbose(u"receiver disconnected%s", {_multiple ? u", waiting for another one" : u""});
        if (!_multiple) {
            // No multiple sessions, terminate here.
            return false;
        }
        // Multiple sessions, close socket and re-open to acquire another receiver.
        stop();
        if (_restart_delay > 0) {
            SleepThread(_restart_delay);
        }
        if (!start()) {
            return false;
        }
    }
}

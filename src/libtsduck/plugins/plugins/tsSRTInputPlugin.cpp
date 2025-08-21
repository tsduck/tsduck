//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2025, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSRTInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsIPProtocols.h"

#if !defined(TS_NO_SRT)
TS_REGISTER_INPUT_PLUGIN(u"srt", ts::SRTInputPlugin);
#endif


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::SRTInputPlugin::SRTInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE,
                                u"Receive TS packets from Secure Reliable Transport (SRT)", u"[options] [[address:]port]",
                                u"srt", u"SRT source time stamp",
                                TSDatagramInputOptions::REAL_TIME | TSDatagramInputOptions::ALLOW_RS204)
{
    _sock.defineArgs(*this);

    option(u"multiple", 'm');
    help(u"multiple", u"When the sender peer disconnects, wait for another one and continue.");

    option<cn::milliseconds>(u"restart-delay");
    help(u"restart-delay", u"With --multiple, wait the specified delay before restarting.");

    // These options are legacy, now use --listener and/or --caller.
    option(u"", 0, IPSOCKADDR, 0, 1);
    help(u"", u"Remote address:port. This is a legacy parameter, now use --caller.");

    option(u"rendezvous", 0, IPSOCKADDR_OA);
    help(u"rendezvous", u"[address:]port", u"Local address and port. This is a legacy option, now use --listener.");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::getOptions()
{
    // Legacy options.
    IPSocketAddress remote;
    IPSocketAddress rendezvous;
    getSocketValue(remote, u"");
    getSocketValue(rendezvous, u"rendezvous");
    _multiple = present(u"multiple");
    getChronoValue(_restart_delay, u"restart-delay");

    // Get command line arguments for superclass and socket.
    return AbstractDatagramInputPlugin::getOptions() &&
           _sock.setAddresses(rendezvous, remote, IPAddress(), *this) &&
           _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    const bool success = AbstractDatagramInputPlugin::start() && _sock.open(NPOS, *this);
    IPSocketAddress local, remote;
    if (success && _sock.getPeers(local, remote, *this)) {
        verbose(u"connected from %s (local: %s)", remote, local);
    }
    return success;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::stop()
{
    AbstractDatagramInputPlugin::stop();
    _sock.close(*this);
    return true;
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::abortInput()
{
    _sock.close(*this);
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource)
{
    timesource = TimeSource::SRT;

    // Loop on restart with multiple sessions.
    for (;;) {
        // Receive packets.
        if (_sock.receive(buffer, buffer_size, ret_size, timestamp, *this)) {
            return true;
        }
        // Receive error.
        if (!_sock.peerDisconnected()) {
            // Actual error, not a clean disconnection from the sender, do not retry, even with --multiple.
            return false;
        }
        verbose(u"sender disconnected%s", _multiple ? u", waiting for another one" : u"");
        if (!_multiple) {
            // No multiple sessions, terminate here.
            return false;
        }
        // Multiple sessions, close socket and re-open to acquire another receiver.
        stop();
        if (_restart_delay > cn::milliseconds::zero()) {
            std::this_thread::sleep_for(_restart_delay);
        }
        if (!start()) {
            return false;
        }
    }
}

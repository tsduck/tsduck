//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Anthony Delannoy
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
                                true), // real-time network reception
    _sock()
{
    _sock.defineArgs(*this);

    // These options are legacy, now use --listener and/or --caller.
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Remote address:port. This is a legacy parameter, now use --caller.");

    option(u"rendezvous", 0, STRING);
    help(u"rendezvous", u"[address:]port", u"Local address and port. This is a legacy option, now use --listener.");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::getOptions()
{
    // Get command line arguments for superclass and socket.
    return AbstractDatagramInputPlugin::getOptions() &&
           _sock.setAddresses(value(u"rendezvous"), value(u""), UString(), *tsp) &&
           _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    return AbstractDatagramInputPlugin::start() && _sock.open(*tsp);
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::stop()
{
    _sock.close(*tsp);
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::abortInput()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    return _sock.receive(buffer, buffer_size, ret_size, timestamp, *tsp);
}

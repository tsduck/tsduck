//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2021, Anthony Delannoy
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

#include "tsSRTOutputPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"srt", ts::SRTOutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::SRTOutputPlugin::REFERENCE = 0;

// Maximum number of TS packets per message in message mode.
#define MAX_PKT_MESSAGE_MODE 7


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::SRTOutputPlugin::SRTOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Secure Reliable Transport (SRT)", u"[options] [address:port]"),
    _multiple(false),
    _restart_delay(0),
    _sock()
{
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
// Output command line options method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::getOptions()
{
    _multiple = present(u"multiple");
    getIntValue(_restart_delay, u"restart-delay", 0);
    return _sock.setAddresses(value(u""), value(u"rendezvous"), *tsp) && _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::start()
{
    if (!_sock.open(*tsp)) {
        _sock.close(*tsp);
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::stop()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::send(const ts::TSPacket* pkt, const ts::TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Loop until all packets are sent.
    while (packet_count > 0) {

        // Send some packets.
        const size_t to_send = (_sock.getMessageApi() && packet_count > MAX_PKT_MESSAGE_MODE) ? MAX_PKT_MESSAGE_MODE : packet_count;
        if (!_sock.send(pkt, to_send * PKT_SIZE, *tsp)) {
            // Send error.
            if (!_sock.peerDisconnected()) {
                // Actual error, not a clean disconnection from the receiver, do not retry, even with --multiple.
                return false;
            }
            tsp->verbose(u"receiver disconnected%s", {_multiple ? u", waiting for another one" : u""});
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

        // Remaining packets.
        pkt += to_send;
        packet_count -= to_send;
    }
    return true;
}

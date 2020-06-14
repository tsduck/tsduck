//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020, Anthony Delannoy
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

#include "tsSRTInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_REGISTER_INPUT_PLUGIN(u"srt", ts::SRTInputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::SRTInputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::SRTInputPlugin::SRTInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE, u"Receive TS packets from Secure Reliable Transport (SRT)", u"[options] [address:]port", u"srt", u"SRT source time stamp"),
    _sock(),
    _mode(SRTSocketMode::CALLER),
    _local_addr(),
    _remote_addr()
{
    _sock.defineArgs(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"The parameter address:port describes the destination for SRT packets. "
         u"The 'address' specifies a unicast IP address. "
         u"It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination SRT port.");

    option(u"rendezvous", 0, ts::Args::STRING);
    help(u"rendezvous", u"address:port", u"Specify local address and port for rendez-vous mode.");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::getOptions()
{
    const UString source(value( u""));
    if (source.empty() || !_remote_addr.resolve(source)) {
        tsp->error(u"Invalid destination address and port: %s", {source});
        return false;
    }

    const UString local(value(u"rendezvous"));
    if (local.empty()) {
        _mode = SRTSocketMode::CALLER;
    }
    else {
        _mode = SRTSocketMode::RENDEZVOUS;
        if (!_local_addr.resolve(local)) {
            tsp->error(u"Invalid local address and port: %s", {local});
            return false;
        }
    }

    // Get command line arguments for superclass and socket.
    return AbstractDatagramInputPlugin::getOptions() && _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::SRTInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    return AbstractDatagramInputPlugin::start() && _sock.open(_mode, _local_addr, _remote_addr, *tsp);
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

bool ts::SRTInputPlugin::receiveDatagram(void* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    return _sock.receive(buffer, buffer_size, ret_size, timestamp, *tsp);
}

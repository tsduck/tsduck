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

#include "tsSRTOutputPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"srt", ts::SRTOutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::SRTOutputPlugin::REFERENCE = 0;

#define MAX_PKT_MESSAGE_MODE 7


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::SRTOutputPlugin::SRTOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Secure Reliable Transport (SRT)", u"[options] address:port"),
    _local_addr(),
    _remote_addr(),
    _pkt_count(0),
    _sock(),
    _mode(SRTSocketMode::LISTENER)
{
    _sock.defineArgs(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specify listening IPv4 and port.");

    option(u"rendezvous", 0, ts::Args::STRING);
    help(u"rendezvous", u"address:port", u"Specify remote address and port for rendez-vous mode.");
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::getOptions(void)
{
    const UString bind_addr(value( u""));
    if (bind_addr.empty() || !_local_addr.resolve(bind_addr)) {
        tsp->error(u"Invalid local address and port: %s", {bind_addr});
        return false;
    }

    const UString remote(value(u"rendezvous"));
    if (remote.empty()) {
        _mode = SRTSocketMode::LISTENER;
    }
    else {
        _mode = SRTSocketMode::RENDEZVOUS;
        if (!_remote_addr.resolve(remote)) {
            tsp->error(u"Invalid remote address and port: %s", {remote});
            return false;
        }
    }

    return _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::start(void)
{
    if (!_sock.open(_mode, _local_addr, _remote_addr, *tsp)) {
        _sock.close(*tsp);
        return false;
    }

    // Other states.
    _pkt_count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::stop(void)
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::SRTOutputPlugin::send(const ts::TSPacket* pkt, const ts::TSPacketMetadata* pkt_data, size_t packet_count)
{
    bool status = false;
    size_t tmp = packet_count;
    const ts::TSPacket* tmp_pkt = pkt;

    while (tmp > 0) {
        const size_t to_send = (_sock.getMessageApi() && tmp > MAX_PKT_MESSAGE_MODE) ? MAX_PKT_MESSAGE_MODE : tmp;
        status = _sock.send(tmp_pkt, to_send * PKT_SIZE, *tsp);
        if (!status) {
            break;
        }
        tmp -= to_send;
        tmp_pkt += to_send;
        _pkt_count += to_send;
    }
    return status;
}

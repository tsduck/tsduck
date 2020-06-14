//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsIPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_REGISTER_INPUT_PLUGIN(u"ip", ts::IPInputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::IPInputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::IPInputPlugin::IPInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE, u"Receive TS packets from UDP/IP, multicast or unicast", u"[options] [address:]port",
                                u"kernel", u"A kernel-provided time-stamp for the packet, when available (Linux only)"),
    _sock(*tsp_)
{
    // Add UDP receiver common options.
    _sock.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::getOptions()
{
    // Get command line arguments for superclass and socket.
    return AbstractDatagramInputPlugin::getOptions() && _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    return AbstractDatagramInputPlugin::start() && _sock.open(*tsp);
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::stop()
{
    _sock.close(*tsp);
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::abortInput()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        _sock.setReceiveTimeoutArg(timeout);
    }
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::receiveDatagram(void* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    SocketAddress sender;
    SocketAddress destination;
    return _sock.receive(buffer, buffer_size, ret_size, sender, destination, tsp, *tsp, &timestamp);
}

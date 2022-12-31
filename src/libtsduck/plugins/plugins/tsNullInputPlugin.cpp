//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsNullInputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_INPUT_PLUGIN(u"null", ts::NullInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NullInputPlugin::NullInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Generate null packets", u"[options] [count]"),
    _max_count(0),
    _count(0),
    _limit(0)
{
    option(u"", 0, UNSIGNED, 0, 1);
    help(u"",
         u"Specify the number of null packets to generate. After the last packet, "
         u"an end-of-file condition is generated. By default, if count is not "
         u"specified, null packets are generated endlessly.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"When the number of null packets is specified, perform a \"joint "
         u"termination\" when completed instead of unconditional termination. "
         u"See \"tsp --help\" for more details on \"joint termination\".");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::NullInputPlugin::getOptions()
{
    tsp->useJointTermination(present(u"joint-termination"));
    getIntValue(_max_count, u"", std::numeric_limits<PacketCounter>::max());
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NullInputPlugin::start()
{
    _count = 0;
    _limit = _max_count;
    return true;
}


//----------------------------------------------------------------------------
// Input is never blocking.
//----------------------------------------------------------------------------

bool ts::NullInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    return true;
}

bool ts::NullInputPlugin::abortInput()
{
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::NullInputPlugin::receive (TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    // If "joint termination" reached for this plugin
    if (_count >= _limit && tsp->useJointTermination()) {
        // Declare terminated
        tsp->jointTerminate();
        // Continue generating null packets until completion of tsp (suppress max packet count)
        _limit = std::numeric_limits<PacketCounter>::max();
    }

    // Fill buffer
    size_t n = 0;
    while (n < max_packets && _count < _limit) {
        _count++;
        buffer[n++] = NullPacket;
    }
    return n;
}

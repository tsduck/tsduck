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
//
//  Transport stream processor shared library:
//  Reduce the bitrate of the TS by dropping null packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ReducePlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(ReducePlugin);
    public:
        // Implementation of plugin API
        ReducePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        int _opt_rempkt;  // rempkt parameter
        int _opt_inpkt;   // inpkt parameter
        int _in_count;    // Input packet count (0 to inpkt)
        int _rem_count;   // Current number of packets to remove
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"reduce", ts::ReducePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ReducePlugin::ReducePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Reduce the TS bitrate by removing stuffing packets", u"[options] rempkt inpkt"),
    _opt_rempkt(0),
    _opt_inpkt(0),
    _in_count(0),
    _rem_count(0)
{
    option(u"", 0, POSITIVE, 2, 2);
    help(u"",
         u"The parameters specify that <rempkt> TS packets must be automatically "
         u"removed after every <inpkt> input TS packets in the transport stream. "
         u"Only stuffing packets can be removed. "
         u"Both <rempkt> and <inpkt> must be non-zero integer values.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ReducePlugin::start()
{
    _opt_rempkt = intValue(u"", 0, 0);
    _opt_inpkt = intValue(u"", 0, 1);
    _in_count = 0;
    _rem_count = 0;
    tsp->debug(u"rempkt = %d, inpkt = %d", {_opt_rempkt, _opt_inpkt});
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ReducePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    assert (_rem_count >= 0);
    assert (_in_count >= 0);
    assert (_in_count <= _opt_inpkt);

    if (_in_count == _opt_inpkt) {
        // It is time to remove packets
        if (_rem_count > 2 * _opt_rempkt) {
            // Overflow, we did not find enough stuffing packets to remove
            tsp->verbose(u"overflow: failed to remove %'d packets", {_rem_count});
        }
        _rem_count += _opt_rempkt;
        _in_count = 0;
    }

    _in_count++;

    if (pkt.getPID() == PID_NULL && _rem_count > 0) {
        _rem_count--;
        return TSP_DROP;
    }
    else {
        return TSP_OK;
    }
}

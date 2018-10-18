//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Boost the bitrate of a PID, stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BoostPIDPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        BoostPIDPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        uint16_t _pid;         // Target PID
        int      _opt_addpkt;  // addpkt in addpkt/inpkt parameter
        int      _opt_inpkt;   // inpkt in addpkt/inpkt parameter
        uint8_t  _next_cc;     // Current continuity counter in PID
        int      _in_count;    // Input packet countdown for next insertion
        int      _add_count;   // Current number of packets to add

        // Inaccessible operations
        BoostPIDPlugin() = delete;
        BoostPIDPlugin(const BoostPIDPlugin&) = delete;
        BoostPIDPlugin& operator=(const BoostPIDPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(boostpid, ts::BoostPIDPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BoostPIDPlugin::BoostPIDPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Boost the bitrate of a PID, stealing stuffing packets", u"[options] pid addpkt inpkt"),
    _pid(PID_NULL),
    _opt_addpkt(0),
    _opt_inpkt(0),
    _next_cc(0),
    _in_count(0),
    _add_count(0)
{
    option(u"", 0, UNSIGNED, 3, 3);
    help(u"",
         u"The first parameter specifies the PID to boost.\n\n"
         u"The second and third parameters specify that <addpkt> TS packets "
         u"must be automatically added after every <inpkt> input TS packets "
         u"in the PID. Both <addpkt> and <inpkt> must be non-zero integer values.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::BoostPIDPlugin::start()
{
    // Get and verify command line arguments
    if ((_pid = intValue<uint16_t>(u"", 0xFFFF, 0)) >= PID_MAX) {
        tsp->error(u"invalid 'pid' parameter");
        return false;
    }
    if ((_opt_addpkt = intValue(u"", 0, 1)) == 0) {
        tsp->error(u"invalid 'addpkt' parameter");
        return false;
    }
    if ((_opt_inpkt = intValue(u"", 0, 2)) == 0) {
        tsp->error(u"invalid 'inpkt' parameter");
        return false;
    }
    tsp->verbose(u"adding %d packets every %d packets on PID %d (0x%X)", {_opt_addpkt, _opt_inpkt, _pid, _pid});

    _next_cc = 0;
    _in_count = 0;
    _add_count = 0;

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BoostPIDPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    if (pid == _pid) {

        // The packet belongs to the target PID. Update counters.

        if (_in_count == 0) {
            // It is time to add more packets
            if (_add_count > 0) {
                // Overflow, we did not find enough stuffing packets to add packets in the target PID.
                tsp->verbose(u"overflow: failed to insert %d packets", {_add_count});
            }
            _add_count += _opt_addpkt;
            _in_count = _opt_inpkt;
        }

        assert (_in_count > 0);
        _in_count--;
    }
    else if (pid == PID_NULL) {

        // This is a stuffing packet. If we don't need any, pass it.

        if (_add_count == 0) {
            return TSP_OK;
        }

        // Insert an empty packet for the target PID:
        // No payload, 184-byte adaptation field

        assert(_add_count > 0);
        _add_count--;

        ::memset(pkt.b, 0xFF, sizeof(pkt.b));

        pkt.b[0] = 0x47;  // sync byte
        PutUInt16(pkt.b + 1, _pid); // PID, no PUSI, no error, no priority
        pkt.b[3] = 0x20;  // adaptation field, no payload
        pkt.b[4] = 183;   // adaptation field length
        pkt.b[5] = 0x00;  // nothing in adaptation field
    }
    else {

        // Neither stuffing nor target PID -> pass it

        return TSP_OK;
    }

    // Update the continuity counter before returning the packet.

    pkt.setCC(_next_cc);
    _next_cc = (_next_cc + 1) & 0x0F;

    return TSP_OK;
}

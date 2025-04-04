//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Boost the bitrate of a PID, stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BoostPIDPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(BoostPIDPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        uint16_t _pid = PID_NULL;  // Target PID
        int      _opt_addpkt = 0;  // addpkt in addpkt/inpkt parameter
        int      _opt_inpkt = 0;   // inpkt in addpkt/inpkt parameter

        // Working data:
        uint8_t  _last_cc = 0;     // Last continuity counter in PID
        int      _in_count = 0;    // Input packet countdown for next insertion
        int      _add_count = 0;   // Current number of packets to add
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"boostpid", ts::BoostPIDPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BoostPIDPlugin::BoostPIDPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Boost the bitrate of a PID, stealing stuffing packets", u"[options] pid addpkt inpkt")
{
    option(u"", 0, UNSIGNED, 3, 3);
    help(u"",
         u"The first parameter specifies the PID to boost.\n\n"
         u"The second and third parameters specify that <addpkt> TS packets "
         u"must be automatically added after every <inpkt> input TS packets "
         u"in the PID. Both <addpkt> and <inpkt> must be non-zero integer values.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::BoostPIDPlugin::getOptions()
{
    if ((_pid = intValue<uint16_t>(u"", 0xFFFF, 0)) >= PID_MAX) {
        error(u"invalid 'pid' parameter");
        return false;
    }
    if ((_opt_addpkt = intValue(u"", 0, 1)) == 0) {
        error(u"invalid 'addpkt' parameter");
        return false;
    }
    if ((_opt_inpkt = intValue(u"", 0, 2)) == 0) {
        error(u"invalid 'inpkt' parameter");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::BoostPIDPlugin::start()
{
    verbose(u"adding %d packets every %d packets on PID %n", _opt_addpkt, _opt_inpkt, _pid);
    _last_cc = 0;
    _in_count = 0;
    _add_count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BoostPIDPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    if (pid == _pid) {

        // The packet belongs to the target PID. Update counters.
        if (_in_count == 0) {
            // It is time to add more packets
            if (_add_count > 0) {
                // Overflow, we did not find enough stuffing packets to add packets in the target PID.
                verbose(u"overflow: failed to insert %d packets", _add_count);
            }
            _add_count += _opt_addpkt;
            _in_count = _opt_inpkt;
        }

        assert (_in_count > 0);
        _in_count--;
        _last_cc = pkt.getCC();
    }
    else if (pid == PID_NULL && _add_count > 0) {

        // Insert an empty packet for the target PID, replacing one stuffing packet.
        // No payload, 184-byte adaptation field
        assert(_add_count > 0);
        _add_count--;

        MemSet(pkt.b, 0xFF, sizeof(pkt.b));

        pkt.b[0] = 0x47;       // sync byte
        PutUInt16(pkt.b + 1, _pid); // PID, no PUSI, no error, no priority
        pkt.b[3] = 0x20;       // adaptation field, no payload
        pkt.b[4] = 183;        // adaptation field length
        pkt.b[5] = 0x00;       // nothing in adaptation field
        pkt.setCC(_last_cc);   // No CC increment without payload -> use last CC
    }

    return TSP_OK;
}

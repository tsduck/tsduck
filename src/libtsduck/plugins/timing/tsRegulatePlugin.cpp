//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRegulatePlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"regulate", ts::RegulatePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RegulatePlugin::RegulatePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Regulate the TS packets flow based on PCR or bitrate", u"[options]")
{
    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specify a bitrate in b/s and regulate (slow down only) the TS packets "
         u"flow according to this bitrate. By default, use the \"input\" bitrate, "
         u"typically resulting from the PCR analysis of the input file.");

    option(u"packet-burst", 'p', POSITIVE);
    help(u"packet-burst",
         u"Number of packets to burst at a time. "
         u"Does not modify the average output bitrate but influence smoothing and CPU load. "
         u"The default is " + UString::Decimal(DEFAULT_PACKET_BURST) + u" packets.");

    option(u"pcr-synchronous");
    help(u"pcr-synchronous",
         u"Regulate the flow based on the Program Clock Reference from the transport stream. "
         u"By default, use a bitrate, not PCR's.");

    option(u"pid-pcr", 0, PIDVAL);
    help(u"pid-pcr",
         u"With --pcr-synchronous, specify the reference PID for PCR's. By default, "
         u"use the first PID containing PCR's.");

    option(u"timestamp-synchronous", 't');
    help(u"timestamp-synchronous",
         u"Regulate the flow based on input timestamps from the input plugin. "
         u"By default, use a bitrate, not input timestamps.");

    option<cn::milliseconds>(u"wait-min", 'w');
    help(u"wait-min",
         u"With --pcr-synchronous and --timestamp-synchronous, specify the minimum wait time in milli-seconds. "
         u"The default is " + UString::Chrono(PCRRegulator::DEFAULT_MIN_WAIT) + u"");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::RegulatePlugin::getOptions()
{
    getValue(_bitrate, u"bitrate", 0);
    getIntValue(_burst, u"packet-burst", DEFAULT_PACKET_BURST);
    getChronoValue(_wait_min, u"wait-min", PCRRegulator::DEFAULT_MIN_WAIT);
    getIntValue(_pid_pcr, u"pid-pcr", PID_NULL);
    _pcr_synchronous = present(u"pcr-synchronous");
    _timestamp_synchronous = present(u"timestamp-synchronous");

    if (present(u"bitrate") + _pcr_synchronous + _timestamp_synchronous > 1) {
        error(u"--bitrate, --pcr-synchronous, and --timestamp-synchronous are mutually exclusive");
        return false;
    }
    if (present(u"pid-pcr") && !_pcr_synchronous) {
        error(u"--pid-pcr cannot be used without --pcr-synchronous");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RegulatePlugin::start()
{
    if (_pcr_synchronous) {
        debug(u"starting PCR-synchronous regulation");
        _regulator = &_pcr_regulator;
        _pcr_regulator.setReferencePID(_pid_pcr);
    }
    else if (_timestamp_synchronous) {
        debug(u"starting timestamp-synchronous regulation");
        _regulator = &_timestamp_regulator;
    }
    else {
        debug(u"starting bitrate-based regulation");
        _regulator = &_bitrate_regulator;
    }

    _regulator->setFixedBitRate(_bitrate);
    _regulator->setMinimimWait(_wait_min);
    _regulator->setBurstPacketCount(_burst);
    return _regulator->reset();
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::RegulatePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _regulator->regulate(pkt, pkt_data, tsp->bitrate());
    return TSP_OK;
}

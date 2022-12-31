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
//
//  Transport stream processor shared library:
//  Edit PCR, PTS and DTS values in various ways.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsEnumeration.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCREditPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PCREditPlugin);
    public:
        // Implementation of plugin API
        PCREditPlugin(TSP*);
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Type of units for PCR, PTS, DTS values.
        enum {UNIT_DEFAULT, UNIT_PCR, UNIT_PTS, UNIT_MILLISEC, UNIT_NANOSEC};

        // Command line options.
        bool    _ignore_scrambled;
        int64_t _add_pcr;
        int64_t _add_pts;
        int64_t _add_dts;
        PIDSet  _pids;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcredit", ts::PCREditPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCREditPlugin::PCREditPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Edit PCR, PTS and DTS values in various ways", u"[options]"),
    _ignore_scrambled(false),
    _add_pcr(0),
    _add_pts(0),
    _add_dts(0),
    _pids()
{
    option(u"add-dts", 0, INT64);
    help(u"add-dts",
         u"Add the specified quantity to all DTS values (can be negative). "
         u"See option --unit for the interpretation of the value.");

    option(u"add-pcr", 0, INT64);
    help(u"add-pcr",
         u"Add the specified quantity to all PCR values (can be negative). "
         u"See option --unit for the interpretation of the value.");

    option(u"add-pts", 0, INT64);
    help(u"add-pts",
         u"Add the specified quantity to all PTS values (can be negative). "
         u"See option --unit for the interpretation of the value.");

    option(u"ignore-scrambled", 'i');
    help(u"ignore-scrambled",
         u"Do not modify PCR values on PID's containing scrambled packets. "
         u"On scrambled PID's, only the PCR's can be modified. "
         u"The PTS and DTS are scrambled and cannot be edited.");

    option(u"negate-pids", 'n');
    help(u"negate-pids",
         u"Negate the selection of --pid options. "
         u"All PID's except the specified ones will have their time-stamps edited.");

    option(u"unit", 'u', Enumeration({
        {u"default",     UNIT_DEFAULT},
        {u"pcr",         UNIT_PCR},
        {u"pts",         UNIT_PTS},
        {u"dts",         UNIT_PTS},
        {u"millisecond", UNIT_MILLISEC},
        {u"nanosecond",  UNIT_NANOSEC},
    }));
    help(u"unit", u"name",
         u"Specify the unit of numeric values for options such as --add-pcr, --add-pts or --add-dts. "
         u"The default unit is \"default\", meaning that each value is a raw number to be applied "
         u"(--add-pcr value is in PCR units, --add-pts value is in PTS units, etc.) "
         u"Otherwise, it is possible to provide uniform values for all options in PCR units, "
         u"PTS/DTS units (the same), nanoseconds or milliseconds. "
         u"The specified values will be converted into the appropriate PCR or PTS/DTS units for each edited field.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies PID's where PCR, DTS and PTS values shall be edited. "
         u"By default, all PID's are modified. Several --pid options may be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PCREditPlugin::getOptions()
{
    _ignore_scrambled = present(u"ignore-scrambled");
    getIntValue(_add_pcr, u"add-pcr", 0);
    getIntValue(_add_pts, u"add-pts", 0);
    getIntValue(_add_dts, u"add-dts", 0);
    getIntValues(_pids, u"pid", true);

    if (present(u"negate-pids")) {
        _pids.flip();
    }

    switch (intValue<int>(u"unit", UNIT_DEFAULT)) {
        case UNIT_PCR:
            _add_pts /= SYSTEM_CLOCK_SUBFACTOR;
            _add_dts /= SYSTEM_CLOCK_SUBFACTOR;
            break;
        case UNIT_PTS:
            _add_pcr *= SYSTEM_CLOCK_SUBFACTOR;
            break;
        case UNIT_MILLISEC:
            _add_pcr = (_add_pcr * SYSTEM_CLOCK_FREQ) / MilliSecPerSec;
            _add_pts = (_add_pts * SYSTEM_CLOCK_SUBFREQ) / MilliSecPerSec;
            _add_dts = (_add_dts * SYSTEM_CLOCK_SUBFREQ) / MilliSecPerSec;
            break;
        case UNIT_NANOSEC:
            _add_pcr = (_add_pcr * SYSTEM_CLOCK_FREQ) / NanoSecPerSec;
            _add_pts = (_add_pts * SYSTEM_CLOCK_SUBFREQ) / NanoSecPerSec;
            _add_dts = (_add_dts * SYSTEM_CLOCK_SUBFREQ) / NanoSecPerSec;
            break;
        default:
            break;
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCREditPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_pids.test(pkt.getPID())) {
        if (_ignore_scrambled && pkt.isScrambled()) {
            // First time we see a scrambled packet on this PID, exclude the PID.
            _pids.reset(pkt.getPID());
        }
        else {
            if (_add_pcr != 0 && pkt.hasPCR()) {
                pkt.setPCR((int64_t(pkt.getPCR()) + _add_pcr) % PCR_SCALE);
            }
            if (_add_pts != 0 && pkt.hasPTS()) {
                pkt.setPTS((int64_t(pkt.getPTS()) + _add_pts) & PTS_DTS_MASK);
            }
            if (_add_dts != 0 && pkt.hasDTS()) {
                pkt.setDTS((int64_t(pkt.getDTS()) + _add_dts) & PTS_DTS_MASK);
            }
        }
    }
    return TSP_OK;
}

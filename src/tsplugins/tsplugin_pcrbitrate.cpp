//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Permanently recompute bitrate based on PCR analysis
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPCRAnalyzer.h"
TSDUCK_SOURCE;

#define DEF_MIN_PCR_CNT  128
#define DEF_MIN_PID        1


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRBitratePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        PCRBitratePlugin(TSP*);
        virtual bool start() override;
        virtual BitRate getBitrate() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        PCRAnalyzer _pcr_analyzer; // PCR analysis context
        BitRate     _bitrate;      // Last remembered bitrate (keep it signed)
        std::string _pcr_name;     // Time stamp type name

        // PCR analysis is done permanently. Typically, the analysis of a
        // constant stream will produce different results quite often. But
        // the results vary by a few bits only. This is a normal behavior
        // which would generate useless activity if reported. Consequently,
        // once a bitrate is statistically computed, we keep it as long as
        // the results are not significantly different. We ignore new results
        // which vary only by less than the following factor.

        static const BitRate REPORT_THRESHOLD = 500000; // 100 b/s on a 50 Mb/s stream

        // Inaccessible operations
        PCRBitratePlugin() = delete;
        PCRBitratePlugin(const PCRBitratePlugin&) = delete;
        PCRBitratePlugin& operator=(const PCRBitratePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::PCRBitratePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRBitratePlugin::PCRBitratePlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Permanently recompute bitrate based on PCR analysis.", "[options]"),
    _pcr_analyzer(),
    _bitrate(0),
    _pcr_name()
{
    option(u"dts",     'd');
    option(u"min-pcr",  0, POSITIVE);
    option(u"min-pid",  0, POSITIVE);

    setHelp(u"Options:\n"
             u"\n"
             u"  -d\n"
             u"  --dts\n"
             u"      Use DTS (Decoding Time Stamps) from video PID's instead of PCR\n"
             u"      (Program Clock Reference) from the transport layer.\n"
             u"\n"
             u"  --help\n"
             u"      Display this help text.\n"
             u"\n"
             u"  --min-pcr value\n"
             u"      Stop analysis when that number of PCR are read from the required\n"
             u"      minimum number of PID (default: " TS_STRINGIFY (DEF_MIN_PCR_CNT) ").\n"
             u"\n"
             u"  --min-pid value\n"
             u"      Minimum number of PID to get PCR from (default: " TS_STRINGIFY (DEF_MIN_PID) ").\n"
             u"\n"
             u"  --version\n"
             u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRBitratePlugin::start()
{
    const size_t min_pcr = intValue<size_t>(u"min-pcr", DEF_MIN_PCR_CNT);
    const size_t min_pid = intValue<size_t>(u"min-pid", DEF_MIN_PID);
    if (present(u"dts")) {
        _pcr_analyzer.resetAndUseDTS (min_pid, min_pcr);
        _pcr_name = "DTS";
    }
    else {
        _pcr_analyzer.reset (min_pid, min_pcr);
        _pcr_name = "PCR";
    }
    _bitrate = 0;
    return true;
}


//----------------------------------------------------------------------------
// Bitrate reporting method
//----------------------------------------------------------------------------

ts::BitRate ts::PCRBitratePlugin::getBitrate()
{
    if (tsp->debug()) {
        tsp->log (Severity::Debug, "getBitrate() called, returning " + Decimal (_bitrate) + " b/s");
    }

    return _bitrate;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRBitratePlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Feed the packet into the PCR analyzer.

    if (_pcr_analyzer.feedPacket (pkt)) {
        // A new bitrate is available, get it and restart analysis
        BitRate new_bitrate = _pcr_analyzer.bitrate188();
        _pcr_analyzer.reset();

        // If the new bitrate is too close to the previous recorded one, no need to signal it.
        if (new_bitrate != _bitrate && (new_bitrate / ::abs (int32_t (new_bitrate) - int32_t (_bitrate))) < REPORT_THRESHOLD) {
            // New bitrate is significantly different, signal it.
            if (tsp->verbose()) {
                tsp->verbose(u"new bitrate from " + _pcr_name + " analysis: " + Decimal (new_bitrate) + " b/s");
            }
            _bitrate = new_bitrate;
            bitrate_changed = true;
        }
    }
    return TSP_OK;
}

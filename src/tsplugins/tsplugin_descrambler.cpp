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
//  DVB-CSA (Common Scrambling Algorithm) Descrambler
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsTSScrambling.h"
#include "tsByteBlock.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DescramblerPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        DescramblerPlugin (TSP*);
        virtual bool start() override;
        virtual Status processPacket (TSPacket&, bool&, bool&) override;

    private:
        TSScrambling _scrambling;
        PIDSet       _pids;

        // Inaccessible operations
        DescramblerPlugin() = delete;
        DescramblerPlugin(const DescramblerPlugin&) = delete;
        DescramblerPlugin& operator=(const DescramblerPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(descrambler, ts::DescramblerPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DescramblerPlugin::DescramblerPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Generic DVB descrambler.", u"[options]"),
    _scrambling(*tsp),
    _pids()
{
    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);

    setHelp(u"General options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Descramble packets with this PID value. Several -p or --pid options may be\n"
            u"      specified. By default, all PID's with scrambled packets are descrambled.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    _scrambling.defineOptions(*this);
    _scrambling.addHelp(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::start()
{
    // Load command line arguments.
    getPIDSet(_pids, u"pid", true);
    if (!_scrambling.loadArgs(*this)) {
        return false;
    }

    // Currently, we need fixed control words.
    if (!_scrambling.hasFixedCW()) {
        tsp->error(u"no control word specified");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DescramblerPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter out PID's which are not descrambled.
    if (!_pids.test(pkt.getPID()) || _scrambling.decrypt(pkt)) {
        return TSP_OK;
    }
    else {
        return TSP_END;
    }
}

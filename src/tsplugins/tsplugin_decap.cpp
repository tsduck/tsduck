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
//  Decapsulate TS packets from one single PID. See also tsplugin_encap.cpp.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPacketDecapsulation.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DecapPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(DecapPlugin);
    public:
        // Implementation of plugin API
        DecapPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool                _ignoreErrors;  // Ignore encapsulation errors.
        PID                 _pid;           // Input PID.
        PacketDecapsulation _decap;         // Decapsulation engine.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"decap", ts::DecapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DecapPlugin::DecapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Decapsulate TS packets from a PID produced by encap plugin", u"[options]"),
    _ignoreErrors(false),
    _pid(PID_NULL),
    _decap()
{
    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore errors such malformed encapsulated stream.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the input PID containing all encapsulated PID's. "
         u"This is a mandatory parameter, there is no default.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DecapPlugin::getOptions()
{
    _ignoreErrors = present(u"ignore-errors");
    _pid = intValue<PID>(u"pid", PID_NULL);
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DecapPlugin::start()
{
    _decap.reset(_pid);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DecapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_decap.processPacket(pkt) || _ignoreErrors || _decap.lastError().empty()) {
        return TSP_OK;
    }
    else {
        tsp->error(_decap.lastError());
        return TSP_END;
    }
}

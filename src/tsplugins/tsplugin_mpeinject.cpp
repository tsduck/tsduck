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
//  Inject MPE (Multi-Protocol Encapsulation) datagrams in a transport stream.
//  See ETSI EN 301 192.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsUDPReceiver.h"
#include "tsMPEPacket.h"
#include "tsThread.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEInjectPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        MPEInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Plugin private fields.
        bool        _replace;  // Replace incoming PID if it exists.
        PID         _pid;      // PID into insert the MPE datagrams.
        UDPReceiver _sock;     // Incoming socket with associated command line options

        // Inaccessible operations
        MPEInjectPlugin() = delete;
        MPEInjectPlugin(const MPEInjectPlugin&) = delete;
        MPEInjectPlugin& operator=(const MPEInjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(mpeinject, ts::MPEInjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MPEInjectPlugin::MPEInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject an incoming UDP stream into MPE (Multi-Protocol Encapsulation).", u"[options] [address:]port"),
    _replace(false),
    _pid(PID_NULL),
    _sock(*tsp_)
{
    option(u"pid",     'p', PIDVAL, 1, 1);
    option(u"replace",  0);

    setHelp(u"\n"
            u"Other options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specify the PID into which the MPE datagrams shall be inserted. Ths is a\n"
            u"      mandatory parameter.\n"
            u"\n"
            u"  --replace\n"
            u"      Replace the target PID if it exists. By default, the plugin only replaces\n"
            "       null packets and tsp stops with an error if incoming packets are found\n"
            "       with the target PID\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    // Add UDP receiver common options and help.
    _sock.defineOptions(*this);
    _sock.addHelp(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::start()
{
    // Get command line arguments.
    _replace = present(u"replace");
    _pid = intValue<PID>(u"pid");
    if (!_sock.load(*this)) {
        return false;
    }

    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::stop()
{
    _sock.close();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MPEInjectPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    return TSP_OK;
}

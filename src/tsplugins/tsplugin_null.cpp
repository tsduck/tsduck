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
//  Null packets input
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NullInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        NullInput(TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual size_t receive(TSPacket*, size_t);

    private:
        PacketCounter _max_count;   // Number of packets to generate
        PacketCounter _count;       // Number of generated packets

        // Inaccessible operations
        NullInput() = delete;
        NullInput(const NullInput&) = delete;
        NullInput& operator=(const NullInput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(ts::NullInput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NullInput::NullInput (TSP* tsp_) :
    InputPlugin(tsp_, "Generate null packets.", "[options] [count]"),
    _max_count(0),
    _count(0)
{
    option ("",                   0,  UNSIGNED, 0, 1);
    option ("joint-termination", 'j');

    setHelp ("Count:\n"
             "  Specify the number of null packets to generate. After the last packet,\n"
             "  an end-of-file condition is generated. By default, if count is not\n"
             "  specified, null packets are generated endlessly.\n"
             "\n"
             "Options:\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -j\n"
             "  --joint-termination\n"
             "      When the number of null packets is specified, perform a \"joint\n"
             "      termination\" when completed instead of unconditional termination.\n"
             "      See \"tsp --help\" for more details on \"joint termination\".\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NullInput::start()
{
    tsp->useJointTermination (present ("joint-termination"));
    _max_count = intValue<PacketCounter> ("", std::numeric_limits<PacketCounter>::max());
    _count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::NullInput::receive (TSPacket* buffer, size_t max_packets)
{
    // If "joint termination" reached for this plugin
    if (_count >= _max_count && tsp->useJointTermination()) {
        // Declare terminated
        tsp->jointTerminate();
        // Continue generating null packets until completion of tsp (suppress max packet count)
        _max_count = std::numeric_limits<PacketCounter>::max();
    }

    // Fill buffer
    size_t n;
    for (n = 0; n < max_packets && _count++ < _max_count; buffer[n++] = NullPacket) {}
    return n;
}

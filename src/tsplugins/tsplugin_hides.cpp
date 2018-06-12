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
//  Output to HiDes modulator devices.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsHiDesDevice.h"
#include "tsCOM.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HiDesOutput: public OutputPlugin
    {
    public:
        // Implementation of plugin API
        HiDesOutput(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, size_t) override;
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;

    private:
        COM _com;    // COM initialization helper

        // Inaccessible operations
        HiDesOutput() = delete;
        HiDesOutput(const HiDesOutput&) = delete;
        HiDesOutput& operator=(const HiDesOutput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_OUTPUT(hides, ts::HiDesOutput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HiDesOutput::HiDesOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a HiDes modulator device", u"[options]"),
    _com(*tsp_)
{
    setHelp(u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::HiDesOutput::start()
{
    // Check that COM was correctly initialized
    if (!_com.isInitialized()) {
        tsp->error(u"COM initialization failure");
        return false;
    }



    return false; //@@@@
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::HiDesOutput::stop()
{
    return false; //@@@@
}


//----------------------------------------------------------------------------
// Bitrate computation method
//----------------------------------------------------------------------------

ts::BitRate ts::HiDesOutput::getBitrate()
{
    return 0; //@@@@
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::HiDesOutput::send(const TSPacket* pkt, size_t packet_count)
{
    return false; //@@@@
}

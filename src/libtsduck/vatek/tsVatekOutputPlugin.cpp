//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022, Richie Chang, Vision Advance Technology Inc. (VATek)
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

#if defined(TS_NO_VATEK)
#include "tsPlatform.h"
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsVatekOutputPluginIsEmpty = true; // Avoid warning about empty module.
#else

#include "tsVatekOutputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_OUTPUT_PLUGIN(u"vatek", ts::VatekOutputPlugin);


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::VatekOutputPlugin::VatekOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a VATEK modulator device", u"[options]")
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Output destructor
//----------------------------------------------------------------------------

ts::VatekOutputPlugin::~VatekOutputPlugin()
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::start()
{
    //@@@@@@@@@@@@
    return false;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::stop()
{
    //@@@@@@@@@@@@
    return false;
}


//----------------------------------------------------------------------------
// Get output bitrate
//----------------------------------------------------------------------------

ts::BitRate ts::VatekOutputPlugin::getBitrate()
{
    //@@@@@@@@@@@@
    return 0;
}

ts::BitRateConfidence ts::VatekOutputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the Vatek device hardware.
    return BitRateConfidence::HARDWARE;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    //@@@@@@@@@@@@
    return false;
}

#endif // TS_NO_VATEK

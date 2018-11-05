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
//  HLS stream input
//
//----------------------------------------------------------------------------

#include "tsAbstractHTTPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsWebRequest.h"
#include "tsWebRequestArgs.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000    // Default size in packet of the inter-thread queue.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HlsInput: public AbstractHTTPInputPlugin
    {
    public:
        // Implementation of plugin API
        HlsInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual void processInput() override;

    private:
        WebRequest     _request;
        WebRequestArgs _web_args;

        // Inaccessible operations
        HlsInput() = delete;
        HlsInput(const HlsInput&) = delete;
        HlsInput& operator=(const HlsInput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(hls, ts::HlsInput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HlsInput::HlsInput(TSP* tsp_) :
    AbstractHTTPInputPlugin(tsp_, u"Read a transport stream from an HLS server", u"[options] url"),
    _request(*tsp),
    _web_args()
{
    _web_args.defineOptions(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specify the URL of an HLS manifest or playlist.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
        u"Specify the maximum number of queued TS packets before their "
        u"insertion into the stream. The default is " +
        UString::Decimal(DEFAULT_MAX_QUEUED_PACKETS) + u".");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::HlsInput::getOptions()
{
    // Decode options.
    _web_args.loadArgs(*this);

    // Resize the inter-thread packet queue.
    setQueueSize(intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS));

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::HlsInput::start()
{
    //@@@@

    // Invoke superclass.
    return AbstractHTTPInputPlugin::start();
}


//----------------------------------------------------------------------------
// Input method. Executed in a separate thread.
//----------------------------------------------------------------------------

void ts::HlsInput::processInput()
{
    //@@@@
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  HTTP stream input
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
    class HttpInput: public AbstractHTTPInputPlugin
    {
        TS_NOBUILD_NOCOPY(HttpInput);
    public:
        // Implementation of plugin API
        HttpInput(TSP*);
        virtual bool getOptions() override;
        virtual void processInput() override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

    private:
        size_t         _repeat_count;
        bool           _ignore_errors;
        MilliSecond    _reconnect_delay;
        UString        _url;
        WebRequestArgs _web_args;
    };
}

TS_REGISTER_INPUT_PLUGIN(u"http", ts::HttpInput);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HttpInput::HttpInput(TSP* tsp_) :
    AbstractHTTPInputPlugin(tsp_, u"Read a transport stream from an HTTP server", u"[options] url"),
    _repeat_count(0),
    _ignore_errors(false),
    _reconnect_delay(0),
    _url(),
    _web_args()
{
    _web_args.defineArgs(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specify the URL from which to read the transport stream.");

    option(u"ignore-errors");
    help(u"ignore-errors",
         u"With --repeat or --infinite, repeat also in case of error. By default, "
         u"repetition stops on error.");

    option(u"infinite", 'i');
    help(u"infinite",
         u"Repeat the playout of the content infinitely (default: only once). "
         u"The URL is re-opened each time and the content may be different.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued TS packets before their "
         u"insertion into the stream. The default is " +
         UString::Decimal(DEFAULT_MAX_QUEUED_PACKETS) + u".");

    option(u"reconnect-delay", 0, UNSIGNED);
    help(u"reconnect-delay",
         u"With --repeat or --infinite, wait the specified number of milliseconds "
         u"before reconnecting. By default, repeat immediately.");

    option(u"repeat", 'r', POSITIVE);
    help(u"repeat", u"count",
         u"Repeat the playout of the content the specified number of times "
         u"(default: only once). The URL is re-opened each time and the content "
         u"may be different.");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::HttpInput::getOptions()
{
    // Decode options.
    _url = value(u"");
    _repeat_count = intValue<size_t>(u"repeat", present(u"infinite") ? std::numeric_limits<size_t>::max() : 1);
    _reconnect_delay = intValue<MilliSecond>(u"reconnect-delay", 0);
    _ignore_errors = present(u"ignore-errors");
    _web_args.loadArgs(duck, *this);

    // Resize the inter-thread packet queue.
    setQueueSize(intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS));

    return true;
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::HttpInput::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        _web_args.receiveTimeout = _web_args.connectionTimeout = timeout;
    }
    return true;
}


//----------------------------------------------------------------------------
// Input method. Executed in a separate thread.
//----------------------------------------------------------------------------

void ts::HttpInput::processInput()
{
    bool ok = true;

    // Create a Web request to download the content.
    WebRequest request(*tsp);
    request.setURL(_url);
    request.setAutoRedirect(true);
    request.setArgs(_web_args);

    // Loop on request count.
    for (size_t count = 0; count < _repeat_count && (ok || _ignore_errors) && !tsp->aborting(); count++) {
        // Wait between reconnections.
        if (count > 0 && _reconnect_delay > 0) {
            SleepThread(_reconnect_delay);
        }
        // Perform one download.
        ok = request.downloadToApplication(this);
    }
}

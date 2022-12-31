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

#include "tsHTTPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsSysUtils.h"

#if !defined(TS_UNIX) || !defined(TS_NO_CURL)
TS_REGISTER_INPUT_PLUGIN(u"http", ts::HTTPInputPlugin);
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HTTPInputPlugin::HTTPInputPlugin(TSP* tsp_) :
    AbstractHTTPInputPlugin(tsp_, u"Read a transport stream from an HTTP server", u"[options] url"),
    _repeat_count(0),
    _ignore_errors(false),
    _reconnect_delay(0),
    _url(),
    _transfer_count(0)
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specify the URL from which to read the transport stream.");

    option(u"ignore-errors");
    help(u"ignore-errors",
         u"With --repeat or --infinite, repeat also in case of error. "
         u"By default, repetition stops on error.");

    option(u"infinite", 'i');
    help(u"infinite",
         u"Repeat the playout of the content infinitely (default: only once). "
         u"The URL is re-opened each time and the content may be different.");

    option(u"reconnect-delay", 0, UNSIGNED);
    help(u"reconnect-delay",
         u"With --repeat or --infinite, wait the specified number of milliseconds before reconnecting. "
         u"By default, repeat immediately.");

    option(u"repeat", 'r', POSITIVE);
    help(u"repeat", u"count",
         u"Repeat the playout of the content the specified number of times (default: only once). "
         u"The URL is re-opened each time and the content may be different.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::HTTPInputPlugin::getOptions()
{
    getValue(_url, u"");
    getIntValue(_repeat_count, u"repeat", present(u"infinite") ? std::numeric_limits<size_t>::max() : 1);
    getIntValue(_reconnect_delay, u"reconnect-delay", 0);
    _ignore_errors = present(u"ignore-errors");
    return AbstractHTTPInputPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::HTTPInputPlugin::start()
{
    _transfer_count = 0;
    return AbstractHTTPInputPlugin::start();
}


//----------------------------------------------------------------------------
// Called by AbstractHTTPInputPlugin to open an URL.
//----------------------------------------------------------------------------

bool ts::HTTPInputPlugin::openURL(WebRequest& request)
{
    // Check if there are any transfer left.
    if (_transfer_count >= _repeat_count) {
        return false;
    }

    // Loop on error retry.
    for (;;) {
        // Give up in case of abort.
        if (tsp->aborting()) {
            return false;
        }

        // Open the URL.
        if (request.open(_url)) {
            _transfer_count++;
            return true;
        }

        // Give up in case of error on first transfer or without error retry.
        if (_transfer_count == 0 || !_ignore_errors || tsp->aborting()) {
            return false;
        }

        // Wait between reconnections.
        if (_reconnect_delay > 0) {
            SleepThread(_reconnect_delay);
        }
    }
}

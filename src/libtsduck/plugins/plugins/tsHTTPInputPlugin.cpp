//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    AbstractHTTPInputPlugin(tsp_, u"Read a transport stream from an HTTP server", u"[options] url")
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

    option<std::chrono::milliseconds>(u"reconnect-delay");
    help(u"reconnect-delay",
         u"With --repeat or --infinite, wait the specified delay before reconnecting. "
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
    getChronoValue(_reconnect_delay, u"reconnect-delay");
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
        if (_reconnect_delay.count() > 0) {
            std::this_thread::sleep_for(_reconnect_delay);
        }
    }
}

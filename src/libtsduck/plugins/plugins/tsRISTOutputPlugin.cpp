//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
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

#include "tsRISTOutputPlugin.h"
#include "tsRISTPluginData.h"
#include "tsPluginRepository.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// This is a real-time plugin in all cases.
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Stubs in the absence of librist.
//----------------------------------------------------------------------------

#if defined(TS_NO_RIST)

#define NORIST_ERROR_MSG u"This version of TSDuck was compiled without RIST support"
#define NORIST_ERROR { tsp->error(NORIST_ERROR_MSG); return false; }

ts::RISTOutputPlugin::RISTOutputPlugin(TSP* t) : AbstractDatagramOutputPlugin(t, u"", u"", NONE), _guts(nullptr) {}
ts::RISTOutputPlugin::~RISTOutputPlugin() {}
bool ts::RISTOutputPlugin::getOptions() NORIST_ERROR
bool ts::RISTOutputPlugin::start() NORIST_ERROR
bool ts::RISTOutputPlugin::stop() NORIST_ERROR
bool ts::RISTOutputPlugin::sendDatagram(const void*, size_t) NORIST_ERROR

#else


//----------------------------------------------------------------------------
// Definition of the implementation.
//----------------------------------------------------------------------------

TS_REGISTER_OUTPUT_PLUGIN(u"rist", ts::RISTOutputPlugin);

class ts::RISTOutputPlugin::Guts
{
     TS_NOBUILD_NOCOPY(Guts);
public:
     RISTPluginData data;
     bool           npd;   // null packet deletion

     // Constructor.
     Guts(Args* args, TSP* tsp) :
         data(args, tsp),
         npd(false)
     {
     }
};


//----------------------------------------------------------------------------
// Output plugin constructor
//----------------------------------------------------------------------------

ts::RISTOutputPlugin::RISTOutputPlugin(TSP* tsp_) :
    AbstractDatagramOutputPlugin(tsp_, u"Send TS packets using Reliable Internet Stream Transport (RIST)", u"[options] url [url...]", NONE),
    _guts(new Guts(this, tsp))
{
    CheckNonNull(_guts);

    option(u"null-packet-deletion", 'n');
    help(u"null-packet-deletion", u"Enable null packet deletion. The receiver needs to support this.");
}

ts::RISTOutputPlugin::~RISTOutputPlugin()
{
    if (_guts != nullptr) {
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Output get command line options
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::getOptions()
{
    _guts->npd = present(u"null-packet-deletion");
    return _guts->data.getOptions(this) && AbstractDatagramOutputPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::start()
{
    if (_guts->data.ctx != nullptr) {
        tsp->error(u"already started");
        return false;
    }

    // Initialize the superclass.
    if (!AbstractDatagramOutputPlugin::start()) {
        return false;
    }

    // Initialize the RIST context.
    tsp->debug(u"calling rist_sender_create, profile: %d", {_guts->data.profile});
    if (::rist_sender_create(&_guts->data.ctx, _guts->data.profile, 0, &_guts->data.log) != 0) {
        tsp->error(u"error in rist_sender_create");
        return false;
    }

    // Add null packet deletion option if requested.
    if (_guts->npd && ::rist_sender_npd_enable(_guts->data.ctx) < 0) {
        tsp->error(u"error setting null-packet deletion");
        _guts->data.cleanup();
        return false;
    }

    // Add all peers to the RIST context.
    if (!_guts->data.addPeers()) {
        return false;
    }

    // Start transmission.
    tsp->debug(u"calling rist_start");
    if (::rist_start(_guts->data.ctx) != 0) {
        tsp->error(u"error starting RIST transmission");
        _guts->data.cleanup();
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::stop()
{
    // Let the superclass send trailing data, if any.
    AbstractDatagramOutputPlugin::stop();

    // Close RIST communication.
    _guts->data.cleanup();
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::sendDatagram(const void* address, size_t size)
{
    // Build a RIST data block describing the data to send.
    ::rist_data_block dblock;
    TS_ZERO(dblock);
    dblock.payload = address;
    dblock.payload_len = size;

    // Send the RIST message.
    const int sent = ::rist_sender_data_write(_guts->data.ctx, &dblock);
    if (sent < 0) {
        tsp->error(u"error sending data to RIST");
        return false;
    }
    else if (size_t(sent) != size) {
        // Don't really know what to do, retry with the rest?
        tsp->warning(u"sent %d bytes to RIST, only %d were written", {size, sent});
    }
    return true;
}

#endif // TS_NO_RIST

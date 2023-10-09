//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRISTOutputPlugin.h"
#include "tsRISTPluginData.h"
#include "tsTSDatagramOutput.h"
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

ts::RISTOutputPlugin::RISTOutputPlugin(TSP* t) : OutputPlugin(t, u"", u""), _guts(nullptr) {}
ts::RISTOutputPlugin::~RISTOutputPlugin() {}
bool ts::RISTOutputPlugin::getOptions() NORIST_ERROR
bool ts::RISTOutputPlugin::start() NORIST_ERROR
bool ts::RISTOutputPlugin::stop() NORIST_ERROR
bool ts::RISTOutputPlugin::send(const TSPacket*, const TSPacketMetadata*, size_t) NORIST_ERROR
bool ts::RISTOutputPlugin::sendDatagram(const void*, size_t, Report&) NORIST_ERROR

#else


//----------------------------------------------------------------------------
// Definition of the implementation.
//----------------------------------------------------------------------------

TS_REGISTER_OUTPUT_PLUGIN(u"rist", ts::RISTOutputPlugin);

class ts::RISTOutputPlugin::Guts
{
     TS_NOBUILD_NOCOPY(Guts);
public:
     TSDatagramOutput datagram;
     RISTPluginData   rist;
     bool             npd;  // null packet deletion

     // Constructor.
     Guts(RISTOutputPlugin* plugin) :
         datagram(TSDatagramOutputOptions::NONE, plugin),
         rist(*plugin->tsp),
         npd(false)
     {
     }
};


//----------------------------------------------------------------------------
// Output plugin constructor
//----------------------------------------------------------------------------

ts::RISTOutputPlugin::RISTOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Reliable Internet Stream Transport (RIST)", u"[options] url [url...]"),
    _guts(new Guts(this))
{
    CheckNonNull(_guts);

    _guts->datagram.defineArgs(*this);
    _guts->rist.defineArgs(*this);

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
    return _guts->datagram.loadArgs(duck, *this) && _guts->rist.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::start()
{
    if (_guts->rist.ctx != nullptr) {
        tsp->error(u"already started");
        return false;
    }

    // Initialize the datagram output.
    if (!_guts->datagram.open(*tsp)) {
        return false;
    }

    // Initialize the RIST context.
    tsp->debug(u"calling rist_sender_create, profile: %d", {_guts->rist.profile});
    if (::rist_sender_create(&_guts->rist.ctx, _guts->rist.profile, 0, &_guts->rist.log) != 0) {
        tsp->error(u"error in rist_sender_create");
        _guts->datagram.close(0, *tsp);
        return false;
    }

    // Add null packet deletion option if requested.
    if (_guts->npd && ::rist_sender_npd_enable(_guts->rist.ctx) < 0) {
        tsp->error(u"error setting null-packet deletion");
        _guts->datagram.close(0, *tsp);
        _guts->rist.cleanup();
        return false;
    }

    // Add all peers to the RIST context.
    if (!_guts->rist.addPeers()) {
        _guts->datagram.close(0, *tsp);
        return false;
    }

    // Start transmission.
    tsp->debug(u"calling rist_start");
    if (::rist_start(_guts->rist.ctx) != 0) {
        tsp->error(u"error starting RIST transmission");
        _guts->datagram.close(0, *tsp);
        _guts->rist.cleanup();
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::stop()
{
    _guts->datagram.close(tsp->bitrate(), *tsp);
    _guts->rist.cleanup();
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    return _guts->datagram.send(packets, packet_count, tsp->bitrate(), *tsp);
}


//----------------------------------------------------------------------------
// Implementation of TSDatagramOutputHandlerInterface: send one datagram.
//----------------------------------------------------------------------------

bool ts::RISTOutputPlugin::sendDatagram(const void* address, size_t size, Report& report)
{
    // Build a RIST data block describing the data to send.
    ::rist_data_block dblock;
    TS_ZERO(dblock);
    dblock.payload = address;
    dblock.payload_len = size;

    // Send the RIST message.
    const int sent = ::rist_sender_data_write(_guts->rist.ctx, &dblock);
    if (sent < 0) {
        report.error(u"error sending data to RIST");
        return false;
    }
    else if (size_t(sent) != size) {
        // Don't really know what to do, retry with the rest?
        report.warning(u"sent %d bytes to RIST, only %d were written", {size, sent});
    }
    return true;
}

#endif // TS_NO_RIST

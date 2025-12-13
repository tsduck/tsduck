//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  DVB-NIP (Native IP) live service extraction.
//
//----------------------------------------------------------------------------

#include "tsAbstractSingleMPEPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEPacket.h"
#include "tsmcastNIPDemux.h"
#include "tshlsPlayList.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts::mcast {
    class NIPExtractPlugin: public AbstractSingleMPEPlugin, private NIPHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(NIPExtractPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual void handleSingleMPEPacket(PCR timestamp, const MPEPacket& mpe) override;

    private:
        // Command line options.
        bool     _use_first_service = false;  // Extract first service.
        UString  _service_name {};            // DVB-NIP service name.
        uint32_t _lcn = 0;                    // DVB-NIP service channel number (if name is empty).

        // Plugin private fields.
        NIPDemux      _demux {duck, this};
        UString       _playlist_url {};       // Playlist of the service.
        hls::PlayList _playlist {};           // Playlist content.

        // Implementation of NIPHandlerInterface.
        virtual void handleNewService(const NIPService& service) override;
        virtual void handleFluteFile(const FluteFile& file) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"nipextract", ts::mcast::NIPExtractPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::mcast::NIPExtractPlugin::NIPExtractPlugin(TSP* tsp_) :
    AbstractSingleMPEPlugin(tsp_, u"DVB-NIP (Native IP) live service extraction", u"[options]", u"DVB-NIP stream")
{
    option(u"lcn", 'l', UINT32);
    help(u"lcn",
         u"Logical channel number of the DVB-NIP service to extract. "
         u"If neither --lcn nor --name are specified, extract the first service that is found.");

    option(u"name", 'n', STRING);
    help(u"name", u"'string'",
         u"Name of the DVB-NIP service to extract. "
         u"The name is case-insensitive and blanks are ignored. "
         u"If neither --lcn nor --name are specified, extract the first service that is found.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::getOptions()
{
    _use_first_service = !present(u"lcn") && !present(u"name");
    getIntValue(_lcn, u"lcn");
    getValue(_service_name, u"name");

    if (present(u"lcn") && present(u"name")) {
        error(u"--lcn and --name are mutually exclusive");
        return false;
    }

    return AbstractSingleMPEPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::start()
{
    _playlist.clear();
    return AbstractSingleMPEPlugin::start() && _demux.reset(FluteDemuxArgs());
}


//----------------------------------------------------------------------------
// MPE packet processing method
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleSingleMPEPacket(PCR timestamp, const MPEPacket& mpe)
{
    _demux.feedPacket(timestamp, mpe.sourceSocket(), mpe.destinationSocket(), mpe.udpMessage(), mpe.udpMessageSize());
}


//----------------------------------------------------------------------------
// Invoked when a new DVB-NIP service is found.
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleNewService(const NIPService& service)
{
    debug(u"new service '%s', LCN: %d", service.service_name, service.channel_number);

    // Ignore new services when ours is already found.
    if (!_playlist_url.empty()) {
        return;
    }

    // Check service name or LCN.
    bool found = false;
    if (!_service_name.empty()) {
        found = _service_name.similar(service.service_name);
    }
    else if (!_use_first_service) {
        found = _lcn == service.channel_number;
    }

    // Look for an instance with HLS playlist, hoping it is made of TS segments.
    if (found || _use_first_service) {
        for (const auto& ins : service.instances) {
            // Does it look like a HLS playlist?
            if (ins.second.media_type.similar(u"application/vnd.apple.mpegurl") || ins.first.ends_with(u".m3u8", CASE_INSENSITIVE)) {
                _playlist_url = ins.first;
                found = true;
                break;
            }
        }
    }

    if (found) {
        if (_playlist_url.empty()) {
            // This was an explicit service, by LCN or by name.
            error(u"no HLS instance found for service '%s', LCN: %d", service.service_name, service.channel_number);
            setError();
        }
        else {
            verbose(u"using service '%s', LCN: %d, provider '%s'", service.service_name, service.channel_number, service.provider_name);
            debug(u"service playlist: %s", _playlist_url);
        }
    }
}


//----------------------------------------------------------------------------
// Invoked for each FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleFluteFile(const FluteFile& file)
{
    // Ignore all files as long as the playlist is unknown.
    if (_playlist_url.empty()) {
        return;
    }

    // Reload the service playlist when found.
    if (file.name() == _playlist_url) {
        // Load or reload the playlist.
        debug(u"reloading playlist %s", file.name());
        bool success = _playlist.isValid() ?
            _playlist.reloadText(file.toText(), false, *this) :
            _playlist.loadText(file.toText(), false, hls::PlayListType::UNKNOWN, *this);
        if (!success) {
            error(u"error reloading service playlist");
            return;
        }
        debug(u"loaded %s playlist", hls::PlayListTypeNames().name(_playlist.type()));

        //@@@ TODO: in case of master playlist, select the intermediate playlist and replace _playlist_url.
    }

    //@@@ TODO: process segments.
}

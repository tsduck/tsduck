//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Identify PID's based on various criteria.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSignalizationDemux.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class IdentifyPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(IdentifyPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _log = false;
        bool             _pmt = false;
        bool             _audio = false;
        bool             _video = false;
        bool             _subtitles = false;
        bool             _scte35 = false;
        bool             _all_service_components = false;
        UString          _service_name {};
        UString          _language {};
        TSPacketLabelSet _set_labels {};

        // Working data:
        uint16_t           _service_id = INVALID_SERVICE_ID;
        PIDSet             _identified_pids {};
        SignalizationDemux _sig_demux {duck, this};

        // Implementation of interfaces.
        virtual void handlePAT(const PAT&, PID) override;
        virtual void handlePMT(const PMT&, PID) override;
        virtual void handleService(uint16_t ts_id, const Service&, const PMT&, bool removed) override;

        // Identify a PID, return true if new.
        bool identifyPID(PID);

        // Identify a new PID with formatted string message.
        template <class... Args>
        void identifyPID(PID pid, const UChar* format, Args&&... args)
        {
            if (identifyPID(pid) && _log) {
                UString fmt;
                fmt.format(u"PID %n: %s", pid, format);
                info(fmt, std::forward<ArgMixIn>(args)...);
            }
        }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"identify", ts::IdentifyPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::IdentifyPlugin::IdentifyPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Identify PID's based on various criteria", u"[options]")
{
    option(u"audio", 'a');
    help(u"audio", u"Identify all PID's carrying audio.");

    option(u"language", 'l', STRING, 0, 0, 3, 3);
    help(u"language", u"language-code",
         u"With --audio or --subtitles, identify PID's carrying the specified language. "
         u"The specified name must be a 3-character ISO-639 language code.");

    option(u"log");
    help(u"log",
         u"Log a message on each newly identified PID. "
         u"This is the default if --set-label is not specified.");

    option(u"pmt", 'p');
    help(u"pmt", u"Identify all PID's carrying PMT's.");

    option(u"scte-35");
    help(u"scte-35", u"Identify all PID's carrying SCTE-35 splice commands.");

    option(u"service", 's', STRING);
    help(u"service", u"name-or-id",
         u"Identify all PID's belonging to the specified service. "
         u"If the argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on all packets of the identified PID's. "
         u"Several --set-label options may be specified, all labels are set on all identified PID's.");

    option(u"subtitles");
    help(u"subtitles", u"Identify all PID's carrying subtitles.");

    option(u"video", 'v');
    help(u"video", u"Identify all PID's carrying video.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::IdentifyPlugin::getOptions()
{
    getIntValues(_set_labels, u"set-label");
    _log = present(u"log") || _set_labels.none();
    _pmt = present(u"pmt");
    _audio = present(u"audio");
    _video = present(u"video");
    _subtitles = present(u"subtitles");
    _scte35 = present(u"scte-35");
    getValue(_service_name, u"service");
    getValue(_language, u"language");

    // Identify all components in a specified service
    _all_service_components = !_audio && !_video && !_subtitles && !_scte35 && !_service_name.empty();

    // Cannot specify incompatible PID content.
    if (_audio + _video + _subtitles + _scte35 + _pmt > 1) {
        error(u"--audio, --video, --subtitles, --scte-35, --pmt are mutually exclusive");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::IdentifyPlugin::start()
{
    // Cleanup state.
    _service_id = INVALID_SERVICE_ID;
    _sig_demux.reset();
    if (_pmt) {
        _sig_demux.addFilteredTableId(TID_PAT);
    }
    else if (!_service_name.empty()) {
        _sig_demux.addFilteredService(_service_name);
    }
    else if (_audio || _video || _subtitles || _scte35) {
        _sig_demux.addFilteredTableId(TID_PMT);
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::IdentifyPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the demux with all incoming packets.
    _sig_demux.feedPacket(pkt);

    // Mark the packets of all identified PID's with the specified labels.
    if (_identified_pids.test(pkt.getPID())) {
        pkt_data.setLabels(_set_labels);
    }
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Identify a PID, return true if new.
//----------------------------------------------------------------------------

bool ts::IdentifyPlugin::identifyPID(PID pid)
{
    if (_identified_pids.test(pid)) {
        // Identified PID is already known, nothing new.
        return false;
    }
    else {
        // New identified PID.
        _identified_pids.set(pid);
        // TODO: additional processing of identified PID's here
        return true;
    }
}


//----------------------------------------------------------------------------
// Invoked by the signalization demux when a PMT is found.
//----------------------------------------------------------------------------

void ts::IdentifyPlugin::handlePAT(const PAT& pat, PID pid)
{
    debug(u"handle PAT on PID %n, %d services", pid, pat.pmts.size());

    // Identify all PMT PID's if no service is selected.
    if (_pmt) {
        for (const auto& it : pat.pmts) {
            if (_service_name.empty() || _service_id == it.first) {
                identifyPID(it.second, u"PMT PID for service %s", _sig_demux.getService(it.first));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the signalization demux when a PMT is found.
//----------------------------------------------------------------------------

void ts::IdentifyPlugin::handlePMT(const PMT& pmt, PID pid)
{
    debug(u"handle PMT on PID %n, service id %n, %d elementary streams", pid, pmt.service_id, pmt.streams.size());

    // If a service is selected, only identify PID's of that service.
    if (_service_name.empty() || pmt.service_id == _service_id) {
        if (_all_service_components) {
            identifyPID(pid, u"PMT PID for service %n", pmt.service_id);
        }
        for (const auto& it : pmt.streams) {
            if (_all_service_components) {
                identifyPID(it.first, u"elementary stream PID for service %s", _sig_demux.getService(pmt.service_id));
            }
            else if (_video && it.second.isVideo(duck)) {
                identifyPID(it.first, u"video PID for service %s", _sig_demux.getService(pmt.service_id));
            }
            else if (_audio && it.second.isAudio(duck) && (_language.empty() || it.second.matchLanguage(duck, _language))) {
                identifyPID(it.first, u"audio PID for service %s", _sig_demux.getService(pmt.service_id));
            }
            else if (_subtitles && it.second.isSubtitles(duck) && (_language.empty() || it.second.matchLanguage(duck, _language))) {
                identifyPID(it.first, u"subtitles PID for service %s", _sig_demux.getService(pmt.service_id));
            }
            else if (_scte35 && it.second.stream_type == ST_SCTE35_SPLICE) {
                identifyPID(it.first, u"SCTE-35 splice PID for service %s", _sig_demux.getService(pmt.service_id));
            }
        }
        if (_all_service_components && pmt.pcr_pid != PID_NULL) {
            // Just in case the PCR PID is not otherwise referenced, eg. not the video PID.
            identifyPID(pmt.pcr_pid, u"PCR PID for service %s", _sig_demux.getService(pmt.service_id));
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the signalization demux when a PMT is found.
//----------------------------------------------------------------------------

void ts::IdentifyPlugin::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    debug(u"handle service %s, PMT valid: %s", service, pmt.isValid());

    // Check if this is the service we identify.
    if (_service_id == INVALID_SERVICE_ID && !_service_name.empty() && service.hasId() && service.match(_service_name)) {
        _service_id = service.getId();
    }

    // Identify PID's in the PMT of the service.
    if (pmt.isValid()) {
        handlePMT(pmt, service.getPMTPID());
    }
}

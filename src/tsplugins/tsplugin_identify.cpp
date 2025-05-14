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
#include "tsEnvironment.h"


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
        bool              _log = false;
        bool              _pmt = false;
        bool              _audio = false;
        bool              _video = false;
        bool              _subtitles = false;
        bool              _scte35 = false;
        bool              _all_service_components = false;
        UString           _service_name {};
        UString           _language {};
        UString           _env_variable {};
        std::set<uint8_t> _stream_types {};
        std::set<REGID>   _registrations {};
        TSPacketLabelSet  _set_labels {};
        TSPacketLabelSet  _all_set_labels {};

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
    option(u"all-set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"all-set-label", u"label1[-label2]",
         u"Set the specified labels on all packets of all PID's in the TS after identifying the first PID. "
         u"See also the option --set-label.");

    option(u"audio", 'a');
    help(u"audio", u"Identify all PID's carrying audio.");

    option(u"language", 'l', STRING, 0, 0, 3, 3);
    help(u"language", u"language-code",
         u"With --audio or --subtitles, identify PID's carrying the specified language. "
         u"The specified name must be a 3-character ISO-639 language code.");

    option(u"log");
    help(u"log",
         u"Log a message on each newly identified PID. "
         u"This is the default when nothing else is specified (--set-label --all-set-label --set-environment-variable).");

    option(u"pmt", 'p');
    help(u"pmt", u"Identify all PID's carrying PMT's.");

    option(u"registration", 0, UINT32, 0, UNLIMITED_COUNT);
    help(u"registration", u"value1[-value2]",
         u"Identify all PID's with a registration descriptor in the PMT containing the specified value (or in the specified range of values). "
         u"Several options --registration are allowed.");

    option(u"scte-35");
    help(u"scte-35", u"Identify all PID's carrying SCTE-35 splice commands.");

    option(u"service", 's', STRING);
    help(u"service", u"name-or-id",
         u"Identify all PID's belonging to the specified service. "
         u"If the argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored.");

    option(u"set-environment-variable", 0, STRING);
    help(u"set-environment-variable", u"name",
         u"When a PID is identified, define the specific environment variable with this PID value. "
         u"This environment variable can be reused in a XML patch file in another plugin, downstream the chain, for instance. "
         u"It is recommended to use this option only when one PID will be identified. "
         u"When several PID's are identified, the environment variable is redefined for each new identified PID "
         u"and using the environment variable later produces different results.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on all packets of the identified PID's. "
         u"Several --set-label options may be specified, all labels are set on all identified PID's.");

    option(u"stream-type", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"stream-type", u"value1[-value2]",
         u"Identify all PID's with any of the specified stream types in the PMT. "
         u"Several options --stream-type are allowed.");

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
    _pmt = present(u"pmt");
    _audio = present(u"audio");
    _video = present(u"video");
    _subtitles = present(u"subtitles");
    _scte35 = present(u"scte-35");
    getValue(_service_name, u"service");
    getValue(_language, u"language");
    getValue(_env_variable, u"set-environment-variable");
    getIntValues(_stream_types, u"stream-type");
    getIntValues(_registrations, u"registration");
    getIntValues(_set_labels, u"set-label");
    getIntValues(_all_set_labels, u"all-set-label");

    // The default operation is logging a message, if nothing else is specified.
    _log = present(u"log") || (_set_labels.none() && _all_set_labels.none() && _env_variable.empty());

    // Identify all components in a service if a service is specified but not more specific selection criteria.
    _all_service_components = !_audio && !_video && !_subtitles && !_scte35 && _stream_types.empty() && _registrations.empty() && !_service_name.empty();

    // Cannot specify incompatible PID content.
    if (_audio + _video + _subtitles + _scte35 + !_stream_types.empty() + !_registrations.empty() + _pmt > 1) {
        error(u"--audio, --video, --subtitles, --scte-35, --stream-type, --registration, --pmt are mutually exclusive");
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
    else if (_audio || _video || _subtitles || _scte35 || !_stream_types.empty() || !_registrations.empty()) {
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

    // Mask all packets in the TS after identifying the first PID.
    if (_identified_pids.any()) {
        pkt_data.setLabels(_all_set_labels);
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
        if (!_env_variable.empty()) {
            if (_identified_pids.any()) {
                // At least one other PID has already been identified.
                warning(u"redefining %s to \"%d\" (was \"%s\")", _env_variable, pid, GetEnvironment(_env_variable));
            }
            SetEnvironment(_env_variable, UString::Decimal(pid, 0, true, UString()));
        }
        _identified_pids.set(pid);
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
            if (!_identified_pids.test(it.first)) {
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
                else if (!_stream_types.empty() && _stream_types.contains(it.second.stream_type)) {
                    identifyPID(it.first, u"PID with stream type %n for service %s", it.second.stream_type, _sig_demux.getService(pmt.service_id));
                }
                if (!_registrations.empty()) {
                    for (auto regid : _registrations) {
                        if (it.second.descs.containsRegistration(regid)) {
                            identifyPID(it.first, u"PID with registration %s for service %s", REGIDName(regid), _sig_demux.getService(pmt.service_id));
                        }
                    }
                }
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

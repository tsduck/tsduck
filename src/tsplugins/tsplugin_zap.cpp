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
//  Zap on one service: Produce a single program transport stream (SPTS)
//  containing only the specified service.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsCASFamily.h"
#include "tsService.h"
#include "tsSignalizationDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ZapPlugin:
        public ProcessorPlugin,
        private SignalizationHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ZapPlugin);
    public:
        // Implementation of plugin API
        ZapPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each PID is described by one byte
        enum : uint8_t {
            TSPID_DROP,   // Remove all packets from this PID
            TSPID_PASS,   // Always pass, unmodified (CAT, TOT/TDT, ATSC PSIP)
            TSPID_PAT,    // PAT, modified
            TSPID_SDT,    // SDT/BAT, modified (SDT Other & BAT removed)
            TSPID_PMT,    // PMT of the service, unmodified
            TSPID_PES,    // A PES component of the service, unmodified
            TSPID_DATA,   // A non-PES component of the service, unmodified
            TSPID_EMM,    // EMM's, unmodified
        };

        // Command line options:
        UString _service_spec;   // Service name or id.
        bool    _spec_by_id;     // Sevice is specified by id (ie. not by name).
        UString _audio;          // Audio language code to keep
        PID     _audio_pid;      // Audio PID to keep
        UString _subtitles;      // Subtitles language code to keep
        bool    _no_subtitles;   // Remove all subtitles
        bool    _no_ecm;         // Remove all ECM PIDs
        bool    _include_cas;    // Include CAS info (CAT & EMM)
        bool    _include_eit;    // Include EIT's for the specified service
        bool    _pes_only;       // Keep PES streams only
        bool    _ignore_absent;  // Do not stop if the service is not present
        Status  _drop_status;    // Status for dropped packets

        // Working data:
        bool               _abort;              // Error (service not found, etc)
        uint8_t            _pat_version;        // Version of next PAT.
        uint8_t            _sdt_version;        // Version of next SDT.
        Service            _service;            // Service name & id
        SignalizationDemux _demux;              // Section demux
        CyclingPacketizer  _pzer_sdt;           // Packetizer for modified SDT
        CyclingPacketizer  _pzer_pat;           // Packetizer for modified PAT
        CyclingPacketizer  _pzer_pmt;           // Packetizer for modified PMT
        EITProcessor       _eit_process;        // Modify EIT's
        uint8_t            _pid_state[PID_MAX]; // Status of each PID.

        // Implementation of SignalizationHandlerInterface.
        virtual void handlePAT(const PAT&, PID) override;
        virtual void handleCAT(const CAT&, PID) override;
        virtual void handlePMT(const PMT&, PID) override;
        virtual void handleSDT(const SDT&, PID) override;
        virtual void handleVCT(const VCT&, PID) override;

        // Send a new PAT.
        void sendNewPAT();

        // Forget all previous components of the service.
        void forgetServiceComponents();

        // Called when the service is not present in the TS.
        void serviceNotPresent(const UChar* table_name);

        // Called when the service id becomes known.
        void setServiceId(uint16_t);

        // Analyze a list of descriptors, looking for CA descriptors.
        // All PIDs which are referenced in CA descriptors are set with the specified state.
        void analyzeCADescriptors(const DescriptorList& dlist, uint8_t pid_state);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"zap", ts::ZapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ZapPlugin::ZapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Zap on one service: Produce an SPTS containing only the specified service", u"[options] service"),
    _service_spec(),
    _spec_by_id(false),
    _audio(),
    _audio_pid(PID_NULL),
    _subtitles(),
    _no_subtitles(false),
    _no_ecm (false),
    _include_cas(false),
    _include_eit(false),
    _pes_only(false),
    _ignore_absent(false),
    _drop_status(TSP_DROP),
    _abort(false),
    _pat_version(0),
    _sdt_version(0),
    _service(),
    _demux(duck, this),
    _pzer_sdt(duck, PID_SDT, CyclingPacketizer::ALWAYS),
    _pzer_pat(duck, PID_PAT, CyclingPacketizer::ALWAYS),
    _pzer_pmt(duck, PID_NULL, CyclingPacketizer::ALWAYS),
    _eit_process(duck, PID_EIT),
    _pid_state()
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specifies the service to keep. If the argument is an integer value (either "
         u"decimal or hexadecimal), it is interpreted as a service id. Otherwise, it "
         u"is interpreted as a service name, as specified in the SDT. The name is not "
         u"case sensitive and blanks are ignored. If the input TS does not contain an "
         u"SDT, use a service id.");

    option(u"audio", 'a', STRING);
    help(u"audio",
         u"Remove all audio components except the specified one. The name is a "
         u"three-letters language code. By default, keep all audio components. "
         u"This option and the --audio-pid option are mutually exclusive.");

    option(u"audio-pid", 0, PIDVAL);
    help(u"audio-pid",
         u"Remove all audio components except the specified audio PID. By default, "
         u"keep all audio components. "
         u"This option and the --audio option are mutually exclusive.");

    option(u"cas", 'c');
    help(u"cas",
         u"Keep Conditional Access System sections (CAT and EMM's). "
         u"Remove them by default. Note that the ECM's for the specified "
         u"service are always kept.");

    option(u"eit");
    help(u"eit",
        u"Keep EIT sections for the specified service. "
        u"EIT sections for other services are removed. "
        u"By default, all EIT's are removed.");

    option(u"ignore-absent", 'i');
    help(u"ignore-absent",
         u"Do not stop if the service does not exist or disappears. "
         u"Continue to pass an empty stream until the service appears or re-appears.");

    option(u"no-ecm", 'e');
    help(u"no-ecm",
         u"Remove all ECM PID's. By default, keep all ECM PID's.");

    option(u"no-subtitles", 'n');
    help(u"no-subtitles",
         u"Remove all subtitles. By default, keep all subtitles.");

    option(u"pes-only", 'p');
    help(u"pes-only",
         u"Keep only the PES elementary streams (audio, video, subtitles). "
         u"Remove all PSI/SI and CAS information.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");

    option(u"subtitles", 't', STRING);
    help(u"subtitles",
         u"Remove all subtitles except the specified one. The name is a "
         u"three-letters language code. By default, keep all subtitles.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::ZapPlugin::getOptions()
{
    duck.loadArgs(*this);

    if (present(u"audio") + present(u"audio-pid") > 1) {
        tsp->error(u"options --audio and --audio-pid are mutually exclusive");
        return false;
    }

    _service_spec = value(u"");
    _audio = value(u"audio");
    _audio_pid = intValue<PID>(u"audio-pid", PID_NULL, 0);
    _subtitles = value(u"subtitles");
    _no_subtitles = present(u"no-subtitles");
    _no_ecm = present(u"no-ecm");
    _include_cas = present(u"cas");
    _include_eit = present(u"eit");
    _pes_only = present(u"pes-only");
    _ignore_absent = present(u"ignore-absent");
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    // Check if service is specified by name or by id.
    Service srv(_service_spec);
    _spec_by_id = srv.hasId();

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ZapPlugin::start()
{
    // Initialize service description.
    _service.clear();
    _service.set(_service_spec);

    // Initialize the demux
    _demux.reset();

    // All PIDs are dropped by default.
    // Selected PIDs will be added when discovered.
    ::memset(_pid_state, TSPID_DROP, sizeof(_pid_state));

    // The TOT and TDT are always passed (same PID).
    _pid_state[PID_TOT] = TSPID_PASS;

    // When the service id is known, we can immediately process the PAT.
    // If the service id is not yet known (only the service name is known), we do not know
    // how to modify the PAT. We will handle it after receiving the DVB-SDT or ATSC-VCT.
    if (_service.hasId()) {
        _demux.addTableId(TID_PAT);
        _demux.addServiceId(_service.getId());
    }

    // Replace the PAT PID with modified PAT.
    _pid_state[PID_PAT] = TSPID_PAT;

    // Always handle the SDT Actual and replace the SDT/BAT PID with modified SDT Actual.
    _demux.addTableId(TID_SDT_ACT);
    _pid_state[PID_SDT] = TSPID_SDT;

    // Handle the ATSC-VCT only when the service is specified by name.
    if (!_spec_by_id) {
        _demux.addTableId(TID_CVCT);
        _demux.addTableId(TID_TVCT);
    }

    // Unlike the DVB-SDT, the ATSC-VCT is not modified to include only the zapped channel
    // because the same PID contains too many distinct tables, some being cycled, some others
    // being one-shot and we do not want to address this complexity here.
    // So, the complete PSIP PID is passed unmodified.
    _pid_state[PID_PSIP] = TSPID_PASS;

    // Include CAT and EMM if required
    if (_include_cas) {
        _demux.addTableId(TID_CAT);
        _pid_state[PID_CAT] = TSPID_PASS;
    }

    // Configure the EIT processor to keep only the selected service.
    _eit_process.reset();
    if (_service.hasId()) {
        _eit_process.keepService(_service);
    }

    // Reset other states
    _abort = false;
    _pat_version = 0;
    _sdt_version = 0;
    _pzer_pat.reset();
    _pzer_pmt.reset();
    _pzer_sdt.reset();

    return true;
}


//----------------------------------------------------------------------------
// Send a new PAT.
//----------------------------------------------------------------------------

void ts::ZapPlugin::sendNewPAT()
{
    // Update a new PAT version.
    _pat_version = (_pat_version + 1) & SVERSION_MASK;

    // Create the new PAT. Set ID id but no NIT PID (this is an SPTS).
    PAT pat(_pat_version, true, _service.getTSId(), PID_NULL);

    // If the service is unknown, send an empty PAT (typically with --ignore-absent).
    if (_service.hasId() && _service.hasPMTPID()) {
        // The service is known, add it in the PAT.
        pat.pmts[_service.getId()] = _service.getPMTPID();
    }

    // Build the list of TS packets containing the new PAT.
    // These packets will replace everything on the PAT PID.
    _pzer_pat.removeAll();
    _pzer_pat.addTable(duck, pat);
}


//----------------------------------------------------------------------------
// Forget all previous components of the service.
//----------------------------------------------------------------------------

void ts::ZapPlugin::forgetServiceComponents()
{
    for (PID pid = 0; pid < PID_MAX; pid++) {
        const uint8_t state = _pid_state[pid];
        if (state == TSPID_PMT || state == TSPID_PES || state == TSPID_DATA) {
            _pid_state[pid] = TSPID_DROP;
        }
    }
}


//----------------------------------------------------------------------------
// Called when the service is not present in the TS.
//----------------------------------------------------------------------------

void ts::ZapPlugin::serviceNotPresent(const UChar* table_name)
{
    if (_ignore_absent) {
        // Service not present is not an error, waiting for it to reappear.
        tsp->verbose(u"service %s not found in %s, waiting for the service...", {_service_spec, table_name});
        // Make sure the service PMT will be notified again if on the same PID.
        _demux.removeAllServiceIds();
        // Forget components that may change when the service reappears.
        forgetServiceComponents();
        _service.clearPMTPID();
        if (_spec_by_id) {
            _service.clearName();
            _demux.addServiceId(_service.getId());
        }
        else {
            _service.clearId();
        }
        // Start sending and empty PAT.
        sendNewPAT();
    }
    else {
        // Service not found is a fatal error.
        tsp->error(u"service %s not found in %s", {_service_spec, table_name});
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Called when the service id becomes known.
//----------------------------------------------------------------------------

void ts::ZapPlugin::setServiceId(uint16_t service_id)
{
    // Ignore case when the service id was already known with same version.
    if (!_service.hasId(service_id)) {

        // Forget previous service.
        _demux.removeAllServiceIds();
        if (_service.hasId()) {
            _service.clearPMTPID();
            forgetServiceComponents();
        }

        // Make sure the new service is monitored.
        _service.setId(service_id);
        _demux.addTableId(TID_PAT);
        _demux.addServiceId(_service.getId());
        tsp->verbose(u"found service %s", {_service});

        // Reset the EIT processor on the new service.
        _eit_process.reset();
        _eit_process.keepService(service_id);

        // If a PAT was already received, check if service is known.
        if (_demux.hasPAT()) {
            handlePAT(_demux.lastPAT(), PID_PAT);
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handlePAT(const PAT& pat, PID pid)
{
    // Remember TS id.
    _service.setTSId(pat.ts_id);

    if (_service.hasId()) {
        // Service id is known, locate it in the PAT.
        const auto it(pat.pmts.find(_service.getId()));
        if (it == pat.pmts.end()) {
            // Service not found in PAT.
            serviceNotPresent(u"PAT");
        }
        else if (!_service.hasPMTPID(it->second)) {
            // Service found with a new PMT PID.
            if (_service.hasPMTPID()) {
                // The PMT PID was previously known but has changed.
                _service.clearPMTPID();
                forgetServiceComponents();
            }
            _service.setPMTPID(it->second);
            tsp->verbose(u"found service id 0x%X, PMT PID is 0x%X", {_service.getId(), _service.getPMTPID()});
            sendNewPAT();
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Service Description Table (SDT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleSDT(const SDT& sdt, PID pid)
{
    // Filter only SDT Actual.
    if (!sdt.isActual()) {
        return;
    }

    // Remember TS id.
    _service.setTSId(sdt.ts_id);
    _service.setONId(sdt.onetw_id);

    // Lookup the VCT only if the service was originally specified by name.
    uint16_t service_id = _service.getId();
    if (!_spec_by_id) {
        // Look for the service by name.
        if (sdt.findService(duck, _service_spec, service_id)) {
            setServiceId(service_id);
        }
        else {
            serviceNotPresent(u"SDT");
        }
    }

    // Cleanup SDT.
    if (!_abort) {
        // Get a modifiable copy of the SDT.
        SDT sdt2(sdt);
        auto it(sdt2.services.find(service_id));
        if (it == sdt2.services.end()) {
            // Service not present in SDT.
            sdt2.services.clear();
        }
        else {
            // Remove other services before zap service
            sdt2.services.erase(sdt2.services.begin(), it);
            // Remove other services after zap service
            it = sdt2.services.begin();
            assert(it != sdt2.services.end());
            assert(it->first == _service.getId());
            sdt2.services.erase(++it, sdt2.services.end());
            assert(sdt2.services.size() == 1);
        }

        // Update a new SDT version. This is useful with --ignore-absent when the service comes and goes.
        _sdt_version = (_sdt_version + 1) & SVERSION_MASK;
        sdt2.version = _sdt_version;

        // Build the list of TS packets containing the new SDT.
        // These packets will replace everything on the SDT/BAT PID.
        _pzer_sdt.removeAll();
        _pzer_sdt.addTable(duck, sdt2);
    }
}


//----------------------------------------------------------------------------
// This method processes an ATSC Virtual Channel Table (VCT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleVCT(const VCT& vct, PID pid)
{
    // Lookup the VCT only if the service was originally specified by name.
    if (!_spec_by_id) {
        // Look for the service by name.
        const auto it(vct.findService(_service_spec));
        if (it == vct.channels.end()) {
            serviceNotPresent(u"VCT");
        }
        else {
            setServiceId(it->second.program_number);
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handlePMT(const PMT& pmt_in, PID pid)
{
    // Filter out any unexpected PMT.
    if (!_service.hasId() || !_service.hasPMTPID() || _service.getId() != pmt_in.service_id || _service.getPMTPID() != pid) {
        return;
    }

    // Forget previous component PID's of the service.
    forgetServiceComponents();

    // Get a modifiable copy of the PMT.
    PMT pmt(pmt_in);

    // Record the PCR PID as a PES component of the service
    if (pmt.pcr_pid != PID_NULL) {
        _pid_state[pmt.pcr_pid] = TSPID_PES;
    }

    // Record or remove ECMs PIDs from the descriptor loop
    if (_no_ecm) {
        // Remove all CA_descriptors
        pmt.descs.removeByTag(DID_CA);
    }
    else {
        // Locate all ECM PID's and record them
        analyzeCADescriptors(pmt.descs, TSPID_DATA);
    }

    // Loop on all elementary streams of the PMT and remove streams we do not
    // need. It is unsafe to iterate through a map and erase elements while
    // iterating. Alternatively, we build a list of PID's (map keys) and
    // we will use the PID's to iterate and erase.
    std::vector<PID> pids;
    pids.reserve(pmt.streams.size());
    for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        pids.push_back(it->first);
    }

    // Number of input and output audio streams
    size_t audio_in = 0;
    size_t audio_out = 0;

    // Now iterate through the list of streams
    for (size_t i = 0; i < pids.size(); ++i) {

        const PID cpid = pids[i];
        PMT::Stream& stream(pmt.streams[cpid]);

        // Check if the component is an audio one with a language
        // other than the one specified on the command line.
        if (stream.isAudio()) {
            audio_in++;
            if (!_audio.empty() && stream.descs.searchLanguage(_audio) >= stream.descs.count()) {
                pmt.streams.erase(cpid);
                continue;
            }
            else if ((_audio_pid != PID_NULL) && (cpid != _audio_pid)) {
                pmt.streams.erase(cpid);
                continue;
            }
            else {
                audio_out++;
            }
        }

        // Check if the component contains subtitles
        size_t subt = stream.descs.searchSubtitle(_subtitles);
        if ((_no_subtitles && subt != stream.descs.count()) || (!_subtitles.empty() && subt > stream.descs.count())) {
            pmt.streams.erase(pid);
            continue;
        }

        // We keep this component, record component PID
        _pid_state[cpid] = uint8_t(IsPES(stream.stream_type) ? TSPID_PES : TSPID_DATA);

        // Record or remove ECMs PIDs from the descriptor loop
        if (_no_ecm) {
            // Remove all CA_descriptors
            stream.descs.removeByTag(DID_CA);
        }
        else {
            // Locate all ECM PID's and record them
            analyzeCADescriptors(stream.descs, TSPID_DATA);
        }
    }

    // Check that the requested audio exists
    if (!_audio.empty() && audio_in > 0 && audio_out == 0) {
        tsp->error(u"audio language \"%s\" not found in PMT", {_audio});
        _abort = true;
        return;
    }
    else if ((_audio_pid != PID_NULL) && (audio_in > 0) && (audio_out == 0)) {
        tsp->error(u"audio pid \"%d\" not found in PMT", {int32_t(_audio_pid)});
        _abort = true;
        return;
    }

    // Build the list of TS packets containing the new PMT.
    // These packets will replace everything on the PMT PID.
    assert(_service.hasPMTPID());
    _pzer_pmt.removeAll();
    _pzer_pmt.setPID(_service.getPMTPID());
    _pzer_pmt.addTable(duck, pmt);

    // Now allow transmission of (modified) packets from PMT PID
    _pid_state[_service.getPMTPID()] = TSPID_PMT;
}


//----------------------------------------------------------------------------
// This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleCAT(const CAT& cat, PID pid)
{
    // Erase all previously known EMM PIDs
    for (size_t epid = 0; epid < PID_MAX; epid++) {
        if (_pid_state[epid] == TSPID_EMM) {
            _pid_state[epid] = TSPID_DROP;
        }
    }

    // Register all new EMM PIDs
    analyzeCADescriptors(cat.descs, TSPID_EMM);
}


//----------------------------------------------------------------------------
// Analyze a list of descriptors, looking for CA descriptors.
//----------------------------------------------------------------------------

void ts::ZapPlugin::analyzeCADescriptors(const DescriptorList& dlist, uint8_t pid_state)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search (DID_CA); index < dlist.count(); index = dlist.search (DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // The fixed part of a CA descriptor is 4 bytes long.
        if (size < 4) {
            continue;
        }
        uint16_t sysid = GetUInt16(desc);
        uint16_t pid = GetUInt16(desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Establish context based on CAS type
        const CASFamily cas = CASFamilyOf(sysid);

        // Record state of main CA pid for this descriptor
        _pid_state[pid] = pid_state;

        // Normally, no PID should be referenced in the private part of
        // a CA descriptor. However, this rule is not followed by the
        // old format of MediaGuard CA descriptors.

        if (cas == CAS_MEDIAGUARD && pid_state == TSPID_EMM && size >= 1 && size != 4) {
            // MediaGuard CA descriptor in the CAT, old format. The test "size != 4"
            // means "not new format" (4 is not a possible size for old format).
            uint8_t nb_opi = *desc;
            desc++; size --;
            while (nb_opi > 0 && size >= 4) {
                pid = GetUInt16(desc) & 0x1FFF;
                desc += 4; size -= 4; nb_opi--;
                // Record state of secondary pid
                _pid_state[pid] = pid_state;
            }
        }
        else if (cas == CAS_MEDIAGUARD && pid_state == TSPID_DATA && size >= 13) {
            // MediaGuard CA descriptor in the PMT.
            desc -= 2; size += 2;
            while (size >= 15) {
                pid = GetUInt16(desc) & 0x1FFF;
                desc += 15; size -= 15;
                // Record state of secondary pid
                _pid_state[pid] = pid_state;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ZapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Filter interesting sections
    _demux.feedPacket(pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Process EIT's (at least when the service id is known).
    if (_include_eit && pid == PID_EIT && _service.hasId()) {
        _eit_process.processPacket(pkt);
        // If the EIT packet has been nullified, we may have to remove it.
        return pkt.getPID() == PID_NULL ? _drop_status : TSP_OK;
    }

    // Remove all non-PES packets if option --pes-only
    if (_pes_only && _pid_state[pid] != TSPID_PES) {
        return _drop_status;
    }

    // Pass, modify or drop the packets
    switch (_pid_state[pid]) {

        case TSPID_DROP:
            // Packet must be dropped or replaced by a null packet
            return _drop_status;

        case TSPID_PASS:
        case TSPID_DATA:
        case TSPID_PES:
        case TSPID_EMM:
            // Packet is passed unmodified.
            return TSP_OK;

        case TSPID_PMT:
            // Replace all PMT packets with modified PMT.
            return _pzer_pmt.getNextPacket(pkt) ? TSP_OK : _drop_status;

        case TSPID_PAT:
            // Replace all PAT packets with modified PAT.
            return _pzer_pat.getNextPacket(pkt) ? TSP_OK : _drop_status;

        case TSPID_SDT:
            // Replace all SDT/BAT packets with modified SDT Actual. SDT Other and BAT are overwritten.
            return _pzer_sdt.getNextPacket(pkt) ? TSP_OK : _drop_status;

        default:
            // Should never get there...
            tsp->error(u"internal error, invalid PID state %d", {_pid_state[pid]});
            return TSP_END;
    }
}

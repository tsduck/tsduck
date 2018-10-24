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
//  Zap on one service: Produce a single program transport stream (SPTS)
//  containing only the specified service.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsCAT.h"
#include "tsSDT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ZapPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        ZapPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Each PID is described by one byte
        enum {
            TSPID_DROP,   // Remove all packets from this PID
            TSPID_PASS,   // Always pass, unmodified (CAT, TOT/TDT)
            TSPID_PAT,    // PAT, modified
            TSPID_SDT,    // SDT/BAT, modified (SDT Other & BAT removed)
            TSPID_PMT,    // PMT of the service, unmodified
            TSPID_PES,    // A PES component of the service, unmodified
            TSPID_DATA,   // A non-PES component of the service, unmodified
            TSPID_EMM,    // EMM's, unmodified
        };

        // Private data
        bool              _abort;              // Error (service not found, etc)
        Service           _service;            // Service name & id
        UString           _audio;              // Audio language code to keep
        PID               _audio_pid;          // Audio PID to keep
        UString           _subtitles;          // Subtitles language code to keep
        bool              _no_subtitles;       // Remove all subtitles
        bool              _no_ecm;             // Remove all ECM PIDs
        bool              _include_cas;        // Include CAS info (CAT & EMM)
        bool              _include_eit;        // Include EIT's for the specified service
        bool              _pes_only;           // Keep PES streams only
        Status            _drop_status;        // Status for dropped packets
        uint8_t           _pid_state[PID_MAX]; // Status of each PID.
        SectionDemux      _demux;              // Section demux
        CyclingPacketizer _pzer_sdt;           // Packetizer for modified SDT
        CyclingPacketizer _pzer_pat;           // Packetizer for modified PAT
        CyclingPacketizer _pzer_pmt;           // Packetizer for modified PMT
        EITProcessor      _eit_process;        // Modify EIT's

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(PAT&);
        void processCAT(CAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);

        // Analyze a list of descriptors, looking for CA descriptors.
        // All PIDs which are referenced in CA descriptors are set with the specified state.
        void analyzeCADescriptors(const DescriptorList& dlist, uint8_t pid_state);

        // Inaccessible operations
        ZapPlugin() = delete;
        ZapPlugin(const ZapPlugin&) = delete;
        ZapPlugin& operator=(const ZapPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(zap, ts::ZapPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ZapPlugin::ZapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Zap on one service: Produce an SPTS containing only the specified service", u"[options] service"),
    _abort(false),
    _service(),
    _audio(),
    _audio_pid(PID_NULL),
    _subtitles(),
    _no_subtitles(false),
    _no_ecm (false),
    _include_cas(false),
    _include_eit(false),
    _pes_only(false),
    _drop_status(TSP_DROP),
    _demux(this),
    _pzer_sdt(PID_SDT, CyclingPacketizer::ALWAYS),
    _pzer_pat(PID_PAT, CyclingPacketizer::ALWAYS),
    _pzer_pmt(PID_NULL, CyclingPacketizer::ALWAYS),
    _eit_process(PID_EIT, tsp_)
{
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
    if (present(u"audio") + present(u"audio-pid") > 1) {
        tsp->error(u"options --audio and --audio-pid are mutually exclusive");
        return false;
    }

    _service.set(value(u""));
    _audio = value(u"audio");
    _audio_pid = intValue<PID>(u"audio-pid", PID_NULL, 0);
    _subtitles = value(u"subtitles");
    _no_subtitles = present(u"no-subtitles");
    _no_ecm = present(u"no-ecm");
    _include_cas = present(u"cas");
    _include_eit = present(u"eit");
    _pes_only = present(u"pes-only");
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ZapPlugin::start()
{
    // All PIDs are dropped by default.
    // Selected PIDs will be added when discovered.
    ::memset(_pid_state, TSPID_DROP, sizeof(_pid_state));

    // The TOT and TDT are always passed.
    assert(PID_TOT == PID_TDT);
    _pid_state[PID_TOT] = TSPID_PASS;

    // Initialize the demux
    _demux.reset();
    _demux.addPID(PID_SDT);

    // When the service id is known, we wait for the PAT. If it is not yet
    // known (only the service name is known), we do not know how to modify
    // the PAT. We will wait for it after receiving the SDT.
    // Packets from PAT PID are analyzed but not passed. When a complete
    // PAT is read, a modified PAT will be transmitted.
    if (_service.hasId()) {
        _demux.addPID(PID_PAT);
    }

    // Include CAT and EMM if required
    if (_include_cas) {
        _demux.addPID(PID_CAT);
        _pid_state[PID_CAT] = TSPID_PASS;
    }

    // Configure the EIT processor to keep only the selected service.
    _eit_process.reset();
    if (_service.hasId()) {
        _eit_process.keepService(_service);
    }

    // Reset other states
    _abort = false;
    _pzer_pat.reset();
    _pzer_pmt.reset();
    _pzer_sdt.reset();

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat(table);
                if (cat.isValid()) {
                    processCAT(cat);
                }
            }
            break;
        }

        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(table);
                if (sdt.isValid()) {
                    processSDT(sdt);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(table);
            if (pmt.isValid() && _service.hasId(pmt.service_id)) {
                processPMT(pmt);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Service Description Table (SDT).
//  We search the service in the SDT. Once we get the service, we rebuild a
//  new SDT containing only one section and only one service (a copy of
//  all descriptors for the service).
//----------------------------------------------------------------------------

void ts::ZapPlugin::processSDT(SDT& sdt)
{
    // Look for the service by name or by service

    bool found;
    uint16_t service_id;

    if (_service.hasName()) {
        found = sdt.findService(_service.getName(), service_id);
    }
    else {
        service_id = _service.getId();
        found = sdt.services.find(service_id) != sdt.services.end();
    }

    // If service not found in SDT and not already found in PAT, error

    if (!found && !_service.hasId(service_id)) {
        tsp->error(u"service \"%s\" not found in SDT", {_service.getName()});
        _abort = true;
        return;
    }

    // If the service id was previously unknown wait for the PAT.
    // If a service id was known but was different, we need to rescan the PAT.

    if (!_service.hasId(service_id)) {

        if (_service.hasId()) {
            // The service was previously known but has changed its service id.
            // We need to rescan the service map. The PMT is reset.
            // All PIDs related to the service are erased.
            for (PID pid = 0; pid < PID_MAX; pid++) {
                uint8_t pstate = _pid_state[pid];
                if (pstate == TSPID_PMT) {
                    _demux.removePID (pid);
                    _pzer_pmt.reset();
                    _pid_state[pid] = TSPID_DROP;
                }
                else if (pstate == TSPID_PES || pstate == TSPID_DATA) {
                    _pid_state[pid] = TSPID_DROP;
                }
            }
        }

        _service.setId(service_id);
        _service.clearPMTPID();

        // Reset the EIT processor.
        _eit_process.reset();
        _eit_process.keepService(service_id);

        // Packets from PAT PID are analyzed but not passed. When a complete
        // PAT is read, a modified PAT will be transmitted.
        _demux.addPID (PID_PAT);
        _pid_state[PID_PAT] = TSPID_DROP;

        tsp->verbose(u"found service \"%s\", service id is 0x%X", {_service.getName(), _service.getId()});
    }

    // Remove all other services from the SDT

    SDT::ServiceMap::iterator it(sdt.services.find(_service.getId()));
    if (it == sdt.services.end()) {
        // Service not present in SDT
        sdt.services.clear();
    }
    else {
        // Remove other services before zap service
        sdt.services.erase(sdt.services.begin(), it);
        // Remove other services after zap service
        it = sdt.services.begin();
        assert(it != sdt.services.end());
        assert(it->first == _service.getId());
        sdt.services.erase(++it, sdt.services.end());
        assert (sdt.services.size() == 1);
    }

    // Build the list of TS packets containing the new SDT.
    // These packets will replace everything on the SDT/BAT PID.

    _pzer_sdt.removeAll();
    _pzer_sdt.addTable(sdt);

    // Now allow transmission of (modified) packets from SDT PID

    _pid_state[PID_SDT] = TSPID_SDT;
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::processPAT(PAT& pat)
{
    // Locate the service in the PAT

    assert(_service.hasId());
    PAT::ServiceMap::iterator it = pat.pmts.find(_service.getId());

    // If service not found, error

    if (it == pat.pmts.end()) {
        tsp->error(u"service id 0x%X not found in PAT", {_service.getId()});
        _abort = true;
        return;
    }

    // If the PMT PID was previously unknown wait for the PMT.
    // If the PMT PID was known but was different, we need to rescan the PMT.

    if (!_service.hasPMTPID(it->second)) {

        if (_service.hasPMTPID()) {
            // The PMT PID was previously known but has changed.
            // We need to rescan the PMT. All PIDs related to the service are erased.
            for (PID pid = 0; pid < PID_MAX; pid++) {
                uint8_t pstate = _pid_state[pid];
                if (pstate == TSPID_PMT) {
                    _demux.removePID (pid);
                    _pzer_pmt.reset();
                    _pid_state[pid] = TSPID_DROP;
                }
                else if (pstate == TSPID_PES || pstate == TSPID_DATA) {
                    _pid_state[pid] = TSPID_DROP;
                }
            }
        }

        _service.setPMTPID(it->second);
        _demux.addPID(it->second);

        tsp->verbose(u"found service id 0x%X, PMT PID is 0x%X", {_service.getId(), _service.getPMTPID()});
    }

    // Remove all other services from the PAT

    pat.pmts.erase (pat.pmts.begin(), it);
    it = pat.pmts.begin();
    assert (it != pat.pmts.end());
    assert (it->first == _service.getId());
    pat.pmts.erase (++it, pat.pmts.end());
    assert (pat.pmts.size() == 1);
    pat.nit_pid = PID_NULL;

    // Build the list of TS packets containing the new PAT.
    // These packets will replace everything on the PAT PID.

    _pzer_pat.removeAll();
    _pzer_pat.addTable (pat);

    // Now allow transmission of (modified) packets from PAT PID

    _pid_state [PID_PAT] = TSPID_PAT;
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::processPMT(PMT& pmt)
{
    // Record the PCR PID as a PES component of the service

    if (pmt.pcr_pid != PID_NULL) {
        _pid_state[pmt.pcr_pid] = TSPID_PES;
    }

    // Record or remove ECMs PIDs from the descriptor loop

    if (_no_ecm) {
        // Remove all CA_descriptors
        pmt.descs.removeByTag (DID_CA);
    }
    else {
        // Locate all ECM PID's and record them
        analyzeCADescriptors (pmt.descs, TSPID_DATA);
    }

    // Loop on all elementary streams of the PMT and remove streams we do not
    // need. It is unsafe to iterate through a map and erase elements while
    // iterating. Alternatively, we build a list of PID's (map keys) and
    // we will use the PID's to iterate and erase.

    std::vector<PID> pids;
    pids.reserve (pmt.streams.size());
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        pids.push_back (it->first);
    }

    // Number of input and output audio streams

    size_t audio_in = 0;
    size_t audio_out = 0;

    // Now iterate through the list of streams

    for (size_t i = 0; i < pids.size(); ++i) {

        const PID pid = pids[i];
        PMT::Stream& stream (pmt.streams[pid]);

        // Check if the component is an audio one with a language
        // other than the one specified on the command line.
        if (stream.isAudio()) {
            audio_in++;
            if (!_audio.empty() && stream.descs.searchLanguage (_audio) >= stream.descs.count()) {
                pmt.streams.erase (pid);
                continue;
            }
            else if ((_audio_pid != PID_NULL) && (pid != _audio_pid)) {
                pmt.streams.erase(pid);
                continue;
            }
            else {
                audio_out++;
            }
        }

        // Check if the component contains subtitles
        size_t subt = stream.descs.searchSubtitle (_subtitles);
        if ((_no_subtitles && subt != stream.descs.count()) || (!_subtitles.empty() && subt > stream.descs.count())) {
            pmt.streams.erase (pid);
            continue;
        }

        // We keep this component, record component PID
        _pid_state[pid] = uint8_t(IsPES(stream.stream_type) ? TSPID_PES : TSPID_DATA);

        // Record or remove ECMs PIDs from the descriptor loop
        if (_no_ecm) {
            // Remove all CA_descriptors
            stream.descs.removeByTag (DID_CA);
        }
        else {
            // Locate all ECM PID's and record them
            analyzeCADescriptors (stream.descs, TSPID_DATA);
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

    assert (_service.hasPMTPID());
    _pzer_pmt.removeAll();
    _pzer_pmt.setPID (_service.getPMTPID());
    _pzer_pmt.addTable (pmt);

    // Now allow transmission of (modified) packets from PMT PID

    _pid_state [_service.getPMTPID()] = TSPID_PMT;
}


//----------------------------------------------------------------------------
//  This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::processCAT(CAT& cat)
{
    // Erase all previously known EMM PIDs
    for (size_t pid = 0; pid < PID_MAX; pid++) {
        if (_pid_state[pid] == TSPID_EMM) {
            _pid_state[pid] = TSPID_DROP;
        }
    }

    // Register all new EMM PIDs
    analyzeCADescriptors (cat.descs, TSPID_EMM);
}


//----------------------------------------------------------------------------
// Analyze a list of descriptors, looking for CA descriptors. All PIDs which
// are referenced in CA descriptors are set with the specified state.
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
        uint16_t sysid = GetUInt16 (desc);
        uint16_t pid = GetUInt16 (desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Establish context based on CAS type
        CASFamily cas = CASFamilyOf (sysid);

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
                pid = GetUInt16 (desc) & 0x1FFF;
                desc += 4; size -= 4; nb_opi--;
                // Record state of secondary pid
                _pid_state[pid] = pid_state;
            }
        }
        else if (cas == CAS_MEDIAGUARD && pid_state == TSPID_DATA && size >= 13) {
            // MediaGuard CA descriptor in the PMT.
            desc -= 2; size += 2;
            while (size >= 15) {
                pid = GetUInt16 (desc) & 0x1FFF;
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

ts::ProcessorPlugin::Status ts::ZapPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
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
            // Packet is passed unmodified
            return TSP_OK;

        case TSPID_PMT:
            // Replace all PMT packets with modified PMT
            _pzer_pmt.getNextPacket (pkt);
            return TSP_OK;

        case TSPID_PAT:
            // Replace all PAT packets with modified PAT
            _pzer_pat.getNextPacket (pkt);
            return TSP_OK;

        case TSPID_SDT:
            // Replace all SDT/BAT packets with modified SDT
            _pzer_sdt.getNextPacket (pkt);
            return TSP_OK;

        default:
            // Should never get there...
            tsp->error(u"internal error, invalid PID state %d", {_pid_state[pid]});
            return TSP_END;
    }
}

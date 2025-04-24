//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Zap on one or more services, remove all other services.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsCVCT.h"
#include "tsTVCT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ZapPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ZapPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each service to keep is described by one structure.
        class ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);
        public:
            // Command line options:
            const UString     service_spec;        // Service name or id.
            bool              spec_by_id = false;  // Service is specified by id (ie. not by name).

            // Working data:
            uint16_t          service_id = 0;      // Service id.
            bool              id_known = false;    // Service id is known.
            CyclingPacketizer pzer_pmt;            // Packetizer for modified PMT.
            std::set<PID>     pids {};             // Set of component PID's.
            PID               pmt_pid = PID_NULL;  // PID for the PMT (PID _NULL if unknown).

            // Constructor:
            ServiceContext(DuckContext& duck, const UString& parameter);
        };
        using ServiceContextPtr = std::shared_ptr<ServiceContext>;
        using ServiceContextVector = std::vector<ServiceContextPtr>;

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

        // Plugin command line options:
        ServiceContextVector _services {};             // Description of services.
        UStringVector        _audio_langs {};          // Audio language codes to keep
        std::set<PID>        _audio_pids {};           // Audio PID's to keep
        UStringVector        _subtitles_langs {};      // Subtitles language codes to keep
        std::set<PID>        _subtitles_pids {};       // Subtitles PID's to keep
        bool                 _no_subtitles = false;    // Remove all subtitles
        bool                 _no_ecm = false;          // Remove all ECM PIDs
        bool                 _include_cas = false;     // Include CAS info (CAT & EMM)
        bool                 _include_eit = false;     // Include EIT's for the specified service
        bool                 _pes_only = false;        // Keep PES streams only
        bool                 _ignore_absent = false;   // Do not stop if a service is not present
        Status               _drop_status = TSP_DROP;  // Status for dropped packets

        // Plugin working data:
        bool                 _abort = false;           // Error (service not found, etc)
        uint8_t              _pat_version = 0;         // Version of next PAT.
        uint8_t              _sdt_version = 0;         // Version of next SDT.
        PAT                  _last_pat {};             // Last received PAT.
        SectionDemux         _demux {duck, this};
        CyclingPacketizer    _pzer_sdt {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer    _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor         _eit_process {duck, PID_EIT};
        uint8_t              _pid_state[PID_MAX] {};   // Status of each PID.

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Handle specific tables.
        void handlePAT(PAT&);
        void handleCAT(CAT&);
        void handlePMT(PMT&, PID);
        void handleSDT(SDT&);
        void handleVCT(VCT&);

        // Send a new PAT.
        void sendNewPAT();

        // Forget all previous components of a service.
        void forgetServiceComponents(ServiceContext& ctx);

        // Called when the service is not present in the TS.
        void serviceNotPresent(ServiceContext& ctx, const UChar* table_name);

        // Called when the service id becomes known.
        void setServiceId(ServiceContext& ctx, uint16_t id);

        // Process ECM PID's from a list of CA descriptors in a PMT (remove or declare ECM PID's).
        void processECM(ServiceContext& ctx, DescriptorList& descs);

        // Analyze a list of descriptors, looking for CA descriptors, collect CA PID's.
        // All PIDs which are referenced in CA descriptors are set with the specified state.
        void analyzeCADescriptors(std::set<PID>& pids, const DescriptorList& descs, uint8_t pid_state);

        // Check if a service component PID (audio or subtitles) shall be kept.
        bool keepComponent(PID pid, const DescriptorList& descs, const UStringVector& languages, const std::set<PID>& pids);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"zap", ts::ZapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ZapPlugin::ZapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Zap on one or more services, remove all other services", u"[options] service ...")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 1, UNLIMITED_COUNT);
    help(u"",
         u"Specifies the services to keep. "
         u"If an argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"Names are not case sensitive and blanks are ignored.");

    option(u"audio", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"audio",
         u"Specify a 3-letter audio language code to keep. "
         u"Several --audio options can be specified. "
         u"All other audio components are removed (unless specified in --audio-pid). "
         u"By default, keep all audio components.");

    option(u"audio-pid", 0, PIDVAL, 0, UNLIMITED_COUNT);
    help(u"audio-pid",
         u"Specify an audio PID to keep. "
         u"Several --audio-pid options can be specified. "
         u"All other audio components are removed (unless specified in --audio). "
         u"By default, keep all audio components.");

    option(u"cas", 'c');
    help(u"cas",
         u"Keep Conditional Access System sections (CAT and EMM's). "
         u"Remove them by default. "
         u"Note that the ECM's for the specified services are always kept.");

    option(u"eit");
    help(u"eit",
        u"Keep EIT sections for the specified services. "
        u"EIT sections for other services are removed. "
        u"By default, all EIT's are removed.");

    option(u"ignore-absent", 'i');
    help(u"ignore-absent",
         u"Do not stop if a specified service does not exist or disappears. "
         u"Continue to pass an empty stream until the service appears or re-appears. "
         u"By default, stop when a service is missing.");

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
         u"Replace excluded packets with stuffing (null packets) instead of removing them. "
         u"Useful to preserve the global TS bitrate.");

    option(u"subtitles", 't', STRING, 0, UNLIMITED_COUNT);
    help(u"subtitles",
         u"Specify a 3-letter subtitles language code to keep. "
         u"Several --subtitles options can be specified. "
         u"All other subtitles components are removed (unless specified in --subtitles-pid). "
         u"By default, keep all subtitles components.");

    option(u"subtitles-pid", 0, PIDVAL, 0, UNLIMITED_COUNT);
    help(u"subtitles-pid",
         u"Specify a subtitles PID to keep. "
         u"Several --subtitles-pid options can be specified. "
         u"All other subtitles components are removed (unless specified in --subtitles). "
         u"By default, keep all subtitles components.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::ZapPlugin::getOptions()
{
    duck.loadArgs(*this);

    // Load list of services.
    _services.clear();
    _services.resize(count(u""));
    for (size_t i = 0; i < _services.size(); ++i) {
        _services[i] = std::make_shared<ServiceContext>(duck, value(u"", u"", i));
    }

    getValues(_audio_langs, u"audio");
    getIntValues(_audio_pids, u"audio-pid");
    getValues(_subtitles_langs, u"subtitles");
    getIntValues(_subtitles_pids, u"subtitles-pid");
    _no_subtitles = present(u"no-subtitles");
    _no_ecm = present(u"no-ecm");
    _include_cas = present(u"cas");
    _include_eit = present(u"eit");
    _pes_only = present(u"pes-only");
    _ignore_absent = present(u"ignore-absent");
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    // Check option conflicts.
    if (_no_subtitles && (!_subtitles_langs.empty() || !_subtitles_pids.empty())) {
        error(u"option --no-subtitles is incompatible with --subtitles and --subtitles-pid");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Context describing a service to keep.
//----------------------------------------------------------------------------

ts::ZapPlugin::ServiceContext::ServiceContext(DuckContext& duck, const UString& parameter) :
    service_spec(parameter),
    pzer_pmt(duck, PID_NULL, CyclingPacketizer::StuffingPolicy::ALWAYS)
{
    id_known = spec_by_id = parameter.toInteger(service_id, UString::DEFAULT_THOUSANDS_SEPARATOR);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ZapPlugin::start()
{
    // Initialize the demux and EIT processor.
    _demux.reset();
    _eit_process.reset();
    _eit_process.removeOther();

    // Initialize service descriptions.
    bool all_ids_known = true;
    for (size_t i = 0; i < _services.size(); ++i) {
        ServiceContext& ctx(*_services[i]);
        ctx.id_known = ctx.spec_by_id;
        ctx.pzer_pmt.reset();
        ctx.pids.clear();
        ctx.pmt_pid = PID_NULL;
        all_ids_known = all_ids_known && ctx.id_known;
        if (ctx.spec_by_id && _include_eit) {
            _eit_process.keepService(ctx.service_id);
        }
    }

    // All PIDs are dropped by default.
    // Selected PIDs will be added when discovered.
    MemSet(_pid_state, TSPID_DROP, sizeof(_pid_state));

    // The TOT and TDT are always passed (same PID).
    _pid_state[PID_TOT] = TSPID_PASS;

    if (all_ids_known) {
        // When all service ids are known, we can immediately process the PAT.
        // If any service id is not yet known (only the service name is known), we do not know
        // how to modify the PAT. We will handle it after receiving the DVB-SDT or ATSC-VCT.
        _demux.addPID(PID_PAT);
    }
    else {
        // Handle the ATSC-VCT only when a service is specified by name.
        // We won't modify the VCT, so there is no need to get them if all service ids are known.
        _demux.addPID(PID_PSIP);
    }

    // Replace the PAT PID with modified PAT.
    _pid_state[PID_PAT] = TSPID_PAT;

    // Always handle the SDT Actual and replace the SDT/BAT PID with modified SDT Actual.
    _demux.addPID(PID_SDT);
    _pid_state[PID_SDT] = TSPID_SDT;

    // Unlike the DVB-SDT, the ATSC-VCT is not modified to include only the zapped channel
    // because the same PID contains too many distinct tables, some being cycled, some others
    // being one-shot and we do not want to address this complexity here.
    // So, the complete PSIP PID is passed unmodified.
    _pid_state[PID_PSIP] = TSPID_PASS;

    // Include CAT and EMM if required
    if (_include_cas) {
        _demux.addPID(PID_CAT);
        _pid_state[PID_CAT] = TSPID_PASS;
    }

    // Reset other states
    _abort = false;
    _pat_version = 0;
    _sdt_version = 0;
    _last_pat.invalidate();
    _pzer_pat.reset();
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

    // Create the new PAT. Set no NIT PID (this is an SPTS in most cases).
    PAT pat(_pat_version, true, _last_pat.ts_id, PID_NULL);

    // Add known services in the PAT.
    // If all services are unknown, send an empty PAT (typically with --ignore-absent).
    for (size_t i = 0; i < _services.size(); ++i) {
        const ServiceContext& ctx(*_services[i]);
        if (ctx.id_known && ctx.pmt_pid != PID_NULL) {
            pat.pmts[ctx.service_id] = ctx.pmt_pid;
        }
    }

    // Build the list of TS packets containing the new PAT.
    // These packets will replace everything on the PAT PID.
    _pzer_pat.removeAll();
    _pzer_pat.addTable(duck, pat);
}


//----------------------------------------------------------------------------
// Forget all previous components of the service.
//----------------------------------------------------------------------------

void ts::ZapPlugin::forgetServiceComponents(ServiceContext& ctx)
{
    // Loop on all known component of the service.
    for (auto pid : ctx.pids) {

        // Loop on all other services to check if the component is shared or not.
        bool shared = false;
        for (size_t i = 0; !shared && i < _services.size(); ++i) {
            // Do not test on the service itself.
            if (_services[i]->id_known && _services[i]->service_id != ctx.service_id) {
                shared = _services[i]->pids.contains(pid);
            }
        }

        // If the PID is not shared, we no longer need to pass it.
        if (!shared) {
            _pid_state[pid] = TSPID_DROP;
        }
    }

    // Clear list of components.
    ctx.pids.clear();
}


//----------------------------------------------------------------------------
// Called when the service is not present in the TS.
//----------------------------------------------------------------------------

void ts::ZapPlugin::serviceNotPresent(ServiceContext& ctx, const UChar* table_name)
{
    if (_ignore_absent) {
        // Service not present is not an error, waiting for it to reappear.
        verbose(u"service %s not found in %s, waiting for the service...", ctx.service_spec, table_name);
        // Make sure the service PMT will be notified again if on the same PID.
        if (ctx.pmt_pid != PID_NULL) {
            _demux.resetPID(ctx.pmt_pid);
            ctx.pmt_pid = PID_NULL;
        }
        // Forget components that may change when the service reappears.
        forgetServiceComponents(ctx);
        // If the service is specified by name, forget its service id.
        ctx.id_known = ctx.spec_by_id;
        // Start sending a PAT without that service.
        sendNewPAT();
    }
    else {
        // Service not found is a fatal error.
        error(u"service %s not found in %s", ctx.service_spec, table_name);
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Called when the service id becomes known.
//----------------------------------------------------------------------------

void ts::ZapPlugin::setServiceId(ServiceContext& ctx, uint16_t service_id)
{
    // Ignore case where the service was already known with the same service id.
    if (!ctx.id_known || ctx.service_id != service_id) {

        verbose(u"found service %s, service id %n", ctx.service_spec, service_id);

        // Forget the previous service.
        ctx.pmt_pid = PID_NULL;
        forgetServiceComponents(ctx);
        if (ctx.id_known && _include_eit) {
            _eit_process.removeService(ctx.service_id);
        }

        // Register the new service.
        ctx.service_id = service_id;
        ctx.id_known = true;
        if (_include_eit) {
            _eit_process.keepService(service_id);
        }

        // At least one service id is known, we need the PAT, if not already done.
        _demux.addPID(TID_PAT);

        // Reprocess last PAT if present to collect new PMT.
        if (_last_pat.isValid()) {
            handlePAT(_last_pat);
        }
    }
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface: receive all new tables.
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();
    switch (table.tableId()) {
        case TID_PAT: {
            PAT pat(duck, table);
            if (pat.isValid() && pid == PID_PAT) {
                handlePAT(pat);
            }
            break;
        }
        case TID_CAT: {
            CAT cat(duck, table);
            if (cat.isValid() && pid == PID_CAT) {
                handleCAT(cat);
            }
            break;
        }
        case TID_PMT: {
            PMT pmt(duck, table);
            if (pmt.isValid()) {
                handlePMT(pmt, pid);
            }
            break;
        }
        case TID_SDT_ACT: {
            SDT sdt(duck, table);
            if (sdt.isValid() && pid == PID_SDT) {
                handleSDT(sdt);
            }
            break;
        }
        case TID_TVCT: {
            TVCT vct(duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct);
            }
            break;
        }
        case TID_CVCT: {
            CVCT vct(duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct);
            }
            break;
        }
        default: {
            // Not interested in that table.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handlePAT(PAT& pat)
{
    // Remember last PAT (unless we reprocess it).
    if (&pat != &_last_pat) {
        _last_pat = pat;
    }

    // Search selected services in the PAT.
    bool need_new_pat = false;
    for (size_t i = 0; i < _services.size(); ++i) {
        ServiceContext& ctx(*_services[i]);
        if (ctx.id_known) {
            // Service id is known, locate it in the PAT.
            const auto it(pat.pmts.find(ctx.service_id));
            if (it == pat.pmts.end()) {
                // Service not found in PAT.
                serviceNotPresent(ctx, u"PAT");
            }
            else if (ctx.pmt_pid != it->second) {
                // Service found with a new PMT PID.
                if (ctx.pmt_pid != PID_NULL) {
                    // The PMT PID was previously known but has changed.
                    forgetServiceComponents(ctx);
                }
                // Need to process the PMT on that PID.
                ctx.pmt_pid = it->second;
                _demux.addPID(ctx.pmt_pid);
                verbose(u"found service id 0x%X, PMT PID is 0x%X", ctx.service_id, ctx.pmt_pid);
                need_new_pat = true;
            }
        }
    }
    if (need_new_pat) {
        sendNewPAT();
    }
}


//----------------------------------------------------------------------------
// This method processes a Service Description Table (SDT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleSDT(SDT& sdt)
{
    // Loop on all selected services, checking those which are specified by name.
    for (size_t i = 0; i < _services.size(); ++i) {
        ServiceContext& ctx(*_services[i]);
        if (!ctx.spec_by_id) {
            uint16_t service_id = 0;
            if (sdt.findService(duck, ctx.service_spec, service_id)) {
                setServiceId(ctx, service_id);
            }
            else {
                serviceNotPresent(ctx, u"SDT");
            }
        }
    }

    // Cleanup SDT. Loop on all services in the SDT, keeping only the selected ones.
    for (auto it = sdt.services.begin(); it != sdt.services.end(); ) {
        // Check if that service is a selected one.
        bool selected = false;
        for (size_t i = 0; !selected && i < _services.size(); ++i) {
            ServiceContext& ctx(*_services[i]);
            selected = ctx.spec_by_id ? ctx.service_id == it->first : ctx.service_spec.similar(it->second.serviceName(duck));
        }
        if (selected) {
            // This service is a selected one, keep it and move to next service in SDT.
            ++it;
        }
        else {
            // This service is not a selected one, remove it from the SDT.
            it = sdt.services.erase(it);
        }
    }

    // Update a new SDT version. This is useful with --ignore-absent when the service comes and goes.
    _sdt_version = (_sdt_version + 1) & SVERSION_MASK;
    sdt.setVersion(_sdt_version);

    // Build the list of TS packets containing the new SDT.
    // These packets will replace everything on the SDT/BAT PID.
    _pzer_sdt.removeAll();
    _pzer_sdt.addTable(duck, sdt);
}


//----------------------------------------------------------------------------
// This method processes an ATSC Virtual Channel Table (VCT).
// The VCT is not modified (not cleaned up of other services) since the PSIP
// contains many other tables, including one-shot tables.
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleVCT(VCT& vct)
{
    // Loop on all selected services, checking those which are specified by name.
    for (size_t i = 0; i < _services.size(); ++i) {
        ServiceContext& ctx(*_services[i]);
        if (!ctx.spec_by_id) {
            const auto it(vct.findService(ctx.service_spec));
            if (it != vct.channels.end()) {
                setServiceId(ctx, it->second.program_number);
            }
            else {
                serviceNotPresent(ctx, u"VCT");
            }
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handlePMT(PMT& pmt, PID pmt_pid)
{
    // Filter out any unexpected PMT.
    ServiceContextPtr ctx;
    for (size_t i = 0; ctx == nullptr && i < _services.size(); ++i) {
        const ServiceContextPtr& ci(_services[i]);
        if (ci->id_known && ci->service_id == pmt.service_id) {
            ctx = ci;
        }
    }
    if (ctx == nullptr) {
        // Not a selected service.
        return;
    }

    // If the PMT PID changed, update it and start a new PAT.
    if (ctx->pmt_pid != pmt_pid) {
        ctx->pmt_pid = pmt_pid;
        sendNewPAT();
    }

    // Forget previous component PID's of the service.
    forgetServiceComponents(*ctx);

    // Record the PCR PID as a PES component of the service
    if (pmt.pcr_pid != PID_NULL) {
        _pid_state[pmt.pcr_pid] = TSPID_PES;
    }

    // Record or remove ECMs PIDs at service level.
    processECM(*ctx, pmt.descs);

    // Loop on all elementary streams of the PMT and remove streams we do not need.
    // Note: no "++i" in "for" expression since "it" can be updated by erase().
    for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ) {

        // Component PID and description.
        const PID cpid = it->first;
        PMT::Stream& stream(it->second);
        bool keep = true;

        // Process audio and subtitles tracks.
        if (stream.isAudio(duck)) {
            keep = keepComponent(cpid, stream.descs, _audio_langs, _audio_pids);
        }
        else if (stream.isSubtitles(duck)) {
            keep = !_no_subtitles && keepComponent(cpid, stream.descs, _subtitles_langs, _subtitles_pids);
        }

        // Keep or remove the component.
        if (keep) {
            // We keep this component, record component PID
            _pid_state[cpid] = uint8_t(StreamTypeIsPES(stream.stream_type) ? TSPID_PES : TSPID_DATA);

            // Record or remove ECMs PIDs at component level.
            processECM(*ctx, stream.descs);

            // Now iterate to next stream.
            ++it;
        }
        else {
            // Remove this component.
            it = pmt.streams.erase(it);
        }
    }

    // Build the list of TS packets containing the new PMT.
    // These packets will replace everything on the PMT PID.
    ctx->pzer_pmt.removeAll();
    ctx->pzer_pmt.setPID(ctx->pmt_pid);
    ctx->pzer_pmt.addTable(duck, pmt);

    // Now allow transmission of (modified) packets from PMT PID
    _pid_state[ctx->pmt_pid] = TSPID_PMT;
}


//----------------------------------------------------------------------------
// This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::ZapPlugin::handleCAT(CAT& cat)
{
    // Erase all previously known EMM PIDs
    for (size_t epid = 0; epid < PID_MAX; epid++) {
        if (_pid_state[epid] == TSPID_EMM) {
            _pid_state[epid] = TSPID_DROP;
        }
    }

    // Register all new EMM PIDs
    std::set<PID> pids;
    analyzeCADescriptors(pids, cat.descs, TSPID_EMM);
}


//----------------------------------------------------------------------------
// Process ECM PID's from a list of CA descriptors in a PMT.
//----------------------------------------------------------------------------

void ts::ZapPlugin::processECM(ServiceContext& ctx, DescriptorList& descs)
{
    if (_no_ecm) {
        // Remove all CA_descriptors
        descs.removeByTag(DID_MPEG_CA);
        descs.removeByTag(DID_ISDB_CA);
    }
    else {
        // Locate all ECM PID's and add them as components of the service.
        analyzeCADescriptors(ctx.pids, descs, TSPID_DATA);
    }
}


//----------------------------------------------------------------------------
// Analyze a list of descriptors, looking for CA descriptors.
//----------------------------------------------------------------------------

void ts::ZapPlugin::analyzeCADescriptors(std::set<PID>& pids, const DescriptorList& descs, uint8_t pid_state)
{
    // Loop on all CA descriptors (MPEG and ISDB).
    for (size_t index = 0; index < descs.size(); ++index) {
        if (descs[index].tag() == DID_MPEG_CA || descs[index].tag() == DID_ISDB_CA) {
            // The fixed part of a CA descriptor is 4 bytes long.
            if (descs[index].payloadSize() >= 4) {
                const uint16_t pid = GetUInt16(descs[index].payload() + 2) & 0x1FFF;
                pids.insert(pid);
                _pid_state[pid] = pid_state;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Check if a service component PID (audio or subtitles) shall be kept.
//----------------------------------------------------------------------------

bool ts::ZapPlugin::keepComponent(PID pid, const DescriptorList& descs, const UStringVector& languages, const std::set<PID>& pids)
{
    // If no language or PID selection, keep all components.
    if (languages.empty() && pids.empty()) {
        return true;
    }

    // Keep explicitly selected PID's.
    if (pids.contains(pid)) {
        return true;
    }

    // Test selected languages one by one.
    for (const auto& it : languages) {
        if (descs.searchLanguage(duck, it) < descs.size()) {
            return true; // language found.
        }
    }

    // No criteria matches, remove the component.
    return false;
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

    // Process EIT's (at least when some service id is known).
    if (_include_eit && pid == PID_EIT && _eit_process.filterServices()) {
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
            // Replace all PMT packets with modified PMT. Look for the right PMT.
            for (size_t i = 0; i < _services.size(); ++i) {
                ServiceContext& ctx(*_services[i]);
                if (ctx.pmt_pid == pid) {
                    return ctx.pzer_pmt.getNextPacket(pkt) ? TSP_OK : _drop_status;
                }
            }
            // If PMT not found, drop the packet.
            return _drop_status;

        case TSPID_PAT:
            // Replace all PAT packets with modified PAT.
            return _pzer_pat.getNextPacket(pkt) ? TSP_OK : _drop_status;

        case TSPID_SDT:
            // Replace all SDT/BAT packets with modified SDT Actual. SDT Other and BAT are overwritten.
            return _pzer_sdt.getNextPacket(pkt) ? TSP_OK : _drop_status;

        default:
            // Should never get there...
            error(u"internal error, invalid PID state %d", _pid_state[pid]);
            return TSP_END;
    }
}

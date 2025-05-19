//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzer.h"
#include "tsT2MIPacket.h"
#include "tsDVB.h"
#include "tsATSC.h"
#include "tsTVCT.h"
#include "tsCVCT.h"
#include "tsServiceDescriptor.h"
#include "tsNetworkNameDescriptor.h"
#include "tsAACDescriptor.h"
#include "tsISO639LanguageDescriptor.h"
#include "tsSubtitlingDescriptor.h"
#include "tsRegistrationDescriptor.h"
#include "tsTeletextDescriptor.h"
#include "tsISDBTInformation.h"
#include "tsBinaryTable.h"
#include "tsDuckContext.h"
#include "tsCAS.h"
#include "tsAlgorithm.h"

// Constant string "Unreferenced"
const ts::UString ts::TSAnalyzer::UNREFERENCED(u"Unreferenced");


//----------------------------------------------------------------------------
// Constructor for the TS analyzer
//----------------------------------------------------------------------------

ts::TSAnalyzer::TSAnalyzer(DuckContext& duck, const BitRate& bitrate_hint, BitRateConfidence bitrate_confidence) :
    _duck(duck),
    _ts_user_bitrate(bitrate_hint),
    _ts_user_br_confidence(bitrate_confidence)
{
    resetSectionDemux();
}

ts::TSAnalyzer::~TSAnalyzer()
{
    this->reset();
}


//----------------------------------------------------------------------------
// Reset the TS analysis context.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::reset()
{
    _modified = false;
    _ts_id.reset();
    _ts_pkt_cnt = 0;
    _invalid_sync = 0;
    _transport_errors = 0;
    _suspect_ignored = 0;
    _ts_isdb_layers.clear();
    _pid_cnt = 0;
    _scrambled_pid_cnt = 0;
    _pcr_pid_cnt = 0;
    _global_pid_cnt = 0;
    _global_scr_pids = 0;
    _global_pkt_cnt = 0;
    _global_bitrate = 0;
    _global_isdb_layers.clear();
    _psisi_pid_cnt = 0;
    _psisi_scr_pids = 0;
    _psisi_pkt_cnt = 0;
    _psisi_bitrate = 0;
    _unref_pid_cnt = 0;
    _unref_scr_pids = 0;
    _unref_pkt_cnt = 0;
    _unref_bitrate = 0;
    _unref_isdb_layers.clear();
    _ts_pcr_bitrate_188 = 0;
    _ts_pcr_bitrate_204 = 0;
    _ts_user_bitrate = 0;
    _ts_user_br_confidence = BitRateConfidence::LOW;
    _ts_bitrate = 0;
    _duration = cn::milliseconds::zero();
    _first_utc = Time::Epoch;
    _last_utc = Time::Epoch;
    _first_local = Time::Epoch;
    _last_local = Time::Epoch;
    _first_tdt = Time::Epoch;
    _last_tdt = Time::Epoch;
    _first_tot = Time::Epoch;
    _last_tot = Time::Epoch;
    _first_stt = Time::Epoch;
    _last_stt = Time::Epoch;
    _country_code.clear();
    _scrambled_services_cnt = 0;
    _tid_present.reset();
    _pids.clear();
    _services.clear();
    _ts_bitrate_sum = 0;
    _ts_bitrate_cnt = 0;
    _preceding_errors = 0;
    _preceding_suspects = 0;
    _pes_demux.reset();
    _t2mi_demux.reset();
    _lcn.clear();
    _dct.invalidate();

    resetSectionDemux();
}


//----------------------------------------------------------------------------
// Reset the section demux.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::resetSectionDemux()
{
    _demux.reset();

    // Specify the PID filters to collect PSI tables.
    // Start with all reserved PID's (ISDB has the highest max reserved PID in MPEG, DVB, ISDB).
    for (PID pid = 0; pid <= PID_ISDB_LAST; ++pid) {
        _demux.addPID(pid);
    }

    // Also add ATSC PSIP PID.
    _demux.addPID(PID_PSIP);
}


//----------------------------------------------------------------------------
// Description of a few known PID's
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::TSAnalyzer::PIDContext::KnownPIDMap);

// Constructor of the singleton.
ts::TSAnalyzer::PIDContext::KnownPIDMap::KnownPIDMap()
{
    //  PID             Description                 Optional  Carry sections
    //  --------------  --------------------------  --------  --------------
    add(PID_NULL,       u"Stuffing",                true,     false);
    add(PID_PAT,        u"PAT",                     false,    true);
    add(PID_CAT,        u"CAT",                     true,     true);
    add(PID_TSDT,       u"TSDT",                    true,     true);
    add(PID_NIT,        u"NIT",                     true,     true);
    add(PID_SDT,        u"SDT/BAT",                 true,     true);
    add(PID_EIT,        u"EIT",                     true,     true);
    add(PID_ISDB_EIT_2, u"ISDB EIT",                true,     true);
    add(PID_ISDB_EIT_3, u"ISDB EIT",                true,     true);
    add(PID_RST,        u"RST",                     true,     true);
    add(PID_TDT,        u"TDT/TOT",                 true,     true);
    add(PID_NETSYNC,    u"Network Synchronization", true,     false);
    add(PID_RNT,        u"RNT (TV-Anytime)",        true,     false);
    add(PID_INBSIGN,    u"Inband Signalling",       true,     false);
    add(PID_MEASURE,    u"Measurement",             true,     false);
    add(PID_DIT,        u"DIT",                     true,     true);
    add(PID_SIT,        u"SIT",                     true,     true);
    add(PID_PSIP,       u"ATSC PSIP",               true,     true);
    add(PID_DCT,        u"ISDB DCT",                true,     true);
    add(PID_PCAT,       u"ISDB PCAT",               true,     true);
    add(PID_SDTT,       u"ISDB SDTT",               true,     true);
    add(PID_SDTT_TER,   u"ISDB SDTT",               true,     true);
    add(PID_BIT,        u"ISDB BIT",                true,     true);
    add(PID_NBIT,       u"ISDB NBIT/LDT",           true,     true);
    add(PID_CDT,        u"ISDB CDT",                true,     true);
    add(PID_AMT,        u"ISDB AMT",                true,     true);
}


//----------------------------------------------------------------------------
// Constructor for the PID context
//----------------------------------------------------------------------------

ts::TSAnalyzer::PIDContext::PIDContext(PID pid_, const UString& description_) :
    pid(pid_),
    description(description_)
{
    // Guess the initial description, based on the PID
    // Global PID's (PAT, CAT, etc) are marked as "referenced" since they
    // should never be considered as orphan PID's. Optional PID's are known
    // PID's which should not appear in the report if no packet are found.

    const KnownPIDMap& kpids(KnownPIDMap::Instance());
    const auto it = kpids.find(pid);
    if (it != kpids.end()) {
        description = it->second.name;
        referenced = true;
        optional = it->second.optional;
        carry_section = it->second.sections;
    }
}


//----------------------------------------------------------------------------
// Return a displayable service or provider name for ServiceContext
//----------------------------------------------------------------------------

ts::UString ts::TSAnalyzer::ServiceContext::getProvider() const
{
    return provider.empty() ? u"(unknown)" : provider;
}

ts::UString ts::TSAnalyzer::ServiceContext::getName() const
{
    if (!name.empty()) {
        return name;
    }
    else if (carry_ssu) {
        return u"(System Software Update)";
    }
    else {
        return u"(unknown)";
    }
}


//----------------------------------------------------------------------------
// Update a service context with information from a descriptor list.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::ServiceContext::update(DuckContext& duck, const DescriptorList& descs)
{
    // Look for a service_descriptor and get service characteristics.
    ServiceDescriptor srv_desc;
    if (descs.search(duck, DID_DVB_SERVICE, srv_desc) < descs.size()) {
        service_type = srv_desc.service_type;
        // Replace names only if they are not empty.
        if (!srv_desc.provider_name.empty()) {
            provider = srv_desc.provider_name;
        }
        if (!srv_desc.service_name.empty()) {
            name = srv_desc.service_name;
        }
    }
}


//----------------------------------------------------------------------------
// Return an XTID context. Allocate a new entry if XTID is not found.
//----------------------------------------------------------------------------

ts::TSAnalyzer::XTIDContextPtr ts::TSAnalyzer::getXTID(const Section& section)
{
    const XTID xtid = section.xtid();
    const PIDContextPtr pc(getPID(section.sourcePID()));
    const auto it = pc->sections.find(xtid);

    if (it != pc->sections.end()) {
        // XTID context found
        return it->second;
    }
    else {
        XTIDContextPtr result = std::make_shared<XTIDContext>(xtid);
        pc->sections[xtid] = result;
        result->first_version = section.version();
        return result;
    }
}


//----------------------------------------------------------------------------
//  Return a PID context. Allocate a new entry if PID not found.
//----------------------------------------------------------------------------

ts::TSAnalyzer::PIDContextPtr ts::TSAnalyzer::getPID(PID pid, const UString& description)
{
    const PIDContextPtr p(_pids[pid]);
    if (p == nullptr) {
        // The PID was not yet used, map entry just created.
        return _pids[pid] = std::make_shared<PIDContext>(pid, description);
    }
    else {
        // If the PID was marked as unreferenced, now use actual description.
        if (p->description == UNREFERENCED && description != UNREFERENCED) {
            p->description = description;
        }
        return p;
    }
}


//----------------------------------------------------------------------------
//  Return a service context. Allocate a new entry if service not found.
//----------------------------------------------------------------------------

ts::TSAnalyzer::ServiceContextPtr ts::TSAnalyzer::getService(uint16_t service_id)
{
    ServiceContextPtr p(_services[service_id]);
    if (p == nullptr) {
        // The service was not yet used, map entry just created.
        return _services[service_id] = std::make_shared<ServiceContext>(service_id);
    }
    else {
        return p;
    }
}


//----------------------------------------------------------------------------
//  Register a PID attribute
//----------------------------------------------------------------------------

void ts::TSAnalyzer::PIDContext::addService(uint16_t service_id)
{
    // The PID now belongs to a service
    referenced = true;

    // Search the service in the list
    if (!services.contains(service_id)) {
        // Service id not found, add it
        services.insert(service_id);
    }
}

void ts::TSAnalyzer::PIDContext::addAttribute(const UString& desc)
{
    if (!desc.similar(description)) {
        AppendUnique(attributes, desc);
    }
}

void ts::TSAnalyzer::PIDContext::addDescriptionOrAttribute(const UString& desc)
{
    if (description.empty() || description == UNREFERENCED) {
        description = desc;
    }
    else {
        AppendUnique(attributes, desc);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an invalid section is received.
// Implementation of InvalidSectionHandlerInterface
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleInvalidSection(SectionDemux&, const DemuxedData& data, Section::Status status)
{
    getPID(data.sourcePID())->inv_sections++;
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Implementation of SectionHandlerInterface
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleSection(SectionDemux&, const Section& section)
{
    XTIDContextPtr etc(getXTID(section));
    const uint8_t version = section.version();

    // Count one section
    etc->section_count++;

    // Section# 0 is used to track tables
    if (section.sectionNumber() == 0) {
        if (etc->table_count++ == 0) {
            // First occurence of table
            etc->first_pkt = _ts_pkt_cnt;
            if (section.isLongSection()) {
                etc->first_version = version;
            }
        }
        else {
            const uint64_t rep = _ts_pkt_cnt - etc->last_pkt;
            if (etc->table_count == 2) {
                // First time we are able to compute an interval
                etc->repetition_ts = etc->min_repetition_ts = etc->max_repetition_ts = rep;
            }
            else {
                if (rep < etc->min_repetition_ts) {
                    etc->min_repetition_ts = rep;
                }
                if (rep > etc->max_repetition_ts) {
                    etc->max_repetition_ts = rep;
                }
                assert(etc->table_count > 2);
                etc->repetition_ts = (_ts_pkt_cnt - etc->first_pkt + (etc->table_count - 1) / 2) / (etc->table_count - 1);
            }
        }
        etc->last_pkt = _ts_pkt_cnt;
        if (section.isLongSection()) {
            etc->versions.set(version);
            etc->last_version = version;
        }
    }

    // On ATSC streams, the System Time Table (STT) shall be read as a section.
    // Due to some ATSC weirdness, they use a long-section format with always
    // the same version number to carry an ever-changing time. As a consequence,
    // it is reported only once as a table.
    if (section.tableId() == TID_STT) {
        const STT stt(_duck, section);
        if (stt.isValid()) {
            analyzeSTT(stt);
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
// (Implementation of TableHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleTable(SectionDemux&, const BinaryTable& table)
{
    const PID pid = table.sourcePID();
    const TID tid = table.tableId();

    // Trace all table ids to identify missing tables
    _tid_present.set(tid);

    // Process specific tables
    switch (tid) {
        case TID_PAT: {
            const PAT pat(_duck, table);
            if (pid == PID_PAT && pat.isValid()) {
                analyzePAT(pat);
            }
            break;
        }
        case TID_CAT: {
            const CAT cat(_duck, table);
            if (pid == PID_CAT && cat.isValid()) {
                analyzeCAT(cat);
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                analyzePMT(pid, pmt);
            }
            break;
        }
        case TID_NIT_ACT: {
            const NIT nit(_duck, table);
            if (nit.isValid()) {
                analyzeNIT(pid, nit);
            }
            break;
        }
        case TID_SDT_ACT: {
            const SDT sdt(_duck, table);
            if (sdt.isValid()) {
                analyzeSDT(sdt);
            }
            break;
        }
        case TID_TDT: {
            const TDT tdt(_duck, table);
            if (tdt.isValid()) {
                analyzeTDT(tdt);
            }
            break;
        }
        case TID_TOT: {
            const TOT tot(_duck, table);
            if (tot.isValid()) {
                analyzeTOT(tot);
            }
            break;
        }
        case TID_MGT: {
            if (pid == PID_PSIP) {
                // Filter by PID to avoid clash with tables with same TID but other standard.
                const MGT mgt(_duck, table);
                if (mgt.isValid()) {
                    analyzeMGT(mgt);
                }
            }
            break;
        }
        case TID_TVCT: {
            if (pid == PID_PSIP) {
                // Filter by PID to avoid clash with tables with same TID but other standard.
                const TVCT tvct(_duck, table);
                if (tvct.isValid()) {
                    analyzeVCT(tvct);
                }
            }
            break;
        }
        case TID_CVCT: {
            if (pid == PID_PSIP) {
                // Filter by PID to avoid clash with tables with same TID but other standard.
                const CVCT cvct(_duck, table);
                if (cvct.isValid()) {
                    analyzeVCT(cvct);
                }
            }
            break;
        }
        case TID_DCT: {
            if (pid == PID_DCT) {
                // Filter by PID to avoid clash with tables with same TID but other standard.
                const DCT dct(_duck, table);
                if (dct.isValid()) {
                    analyzeDCT(dct);
                }
            }
            break;
        }
        case TID_ASTRA_SGT: {
            const SGT sgt(_duck, table);
            if (sgt.isValid()) {
                analyzeSGT(sgt, pid);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Analyze a PAT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzePAT(const PAT& pat)
{
    // Get the transport stream id
    _ts_id = pat.ts_id;

    // Get all PMT PID's for all services
    for (auto& it : pat.pmts) {
        uint16_t service_id(it.first);
        PID pmt_pid(it.second);
        // Register the PMT PID
        PIDContextPtr ps(getPID(pmt_pid));
        ps->description = u"PMT";
        ps->addService(service_id);
        ps->is_pmt_pid = true;
        ps->carry_section = true;
        // Add a filter on the referenced PID to get the PMT
        _demux.addPID(pmt_pid);
        // Describe the service
        ServiceContextPtr svp(getService(service_id));
        svp->pmt_pid = pmt_pid;
    }

    // If a DCT was waiting for the TS id to be analyzed, do it now.
    if (_dct.isValid()) {
        analyzeDCT(_dct);
        _dct.invalidate();
    }
}


//----------------------------------------------------------------------------
// Analyze a CAT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeCAT(const CAT& cat)
{
    // Analyze the CA descriptors to find EMM PIDs
    analyzeDescriptors(cat.descs);
}


//----------------------------------------------------------------------------
// Analyze a PMT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzePMT(PID pid, const PMT& pmt)
{
    // Count the number of PMT's on this PID
    PIDContextPtr ps(getPID(pid));
    ps->pmt_cnt++;

    // Get service description
    ServiceContextPtr svp(getService(pmt.service_id));

    // Check that this PMT was expected on this PID
    if (svp->pmt_pid != pid) {
        // PAT/PMT inconsistency: Found a PMT on a PID which was not
        // referenced as a PMT PID in the PAT.
        ps->addService(pmt.service_id);
        ps->description = u"PMT";
    }

    // Locate PCR PID
    if (pmt.pcr_pid != 0 && pmt.pcr_pid != PID_NULL) {
        svp->pcr_pid = pmt.pcr_pid;
        // This PID is the PCR PID for this service. Initial description
        // will normally be replaced later by "Audio", "Video", etc.
        // Some encoders, however, generate a dedicated PID for PCR's.
        ps = getPID(pmt.pcr_pid, u"PCR (not otherwise referenced)");
        ps->is_pcr_pid = true;
        ps->addService(pmt.service_id);
    }

    // Process "program info" list of descriptors.
    analyzeDescriptors(pmt.descs, svp.get());

    // Some broadcasters incorrectly place the service_descriptor in the PMT instead of the SDT.
    svp->update(_duck, pmt.descs);

    // Process all "elementary stream info"
    for (auto& it : pmt.streams) {
        const PID es_pid = it.first;
        const PMT::Stream& stream(it.second);
        ps = getPID(es_pid);
        ps->addService(pmt.service_id);
        ps->stream_type = stream.stream_type;
        ps->carry_audio = ps->carry_audio || StreamTypeIsAudio(stream.stream_type, pmt.descs) || StreamTypeIsAudio(stream.stream_type, stream.descs);
        ps->carry_video = ps->carry_video || StreamTypeIsVideo(stream.stream_type);
        ps->carry_pes = ps->carry_pes || StreamTypeIsPES(stream.stream_type);
        if (!ps->carry_section && !ps->carry_t2mi && StreamTypeIsSection(stream.stream_type)) {
            ps->carry_section = true;
            _demux.addPID(es_pid);
        }

        // AAC audio streams have the same outer syntax as MPEG-2 Audio.
        if (ps->audio2.isValid() && (ps->stream_type == ST_MPEG1_AUDIO || ps->stream_type == ST_MPEG2_AUDIO)) {
            // We are sure that the stream is MPEG 1/2 Audio.
            ps->addAttribute(ps->audio2.toString());
        }

        // If any registration id applies to the stream type, it shall come from the program-level descriptor list.
        ps->description = StreamTypeName(stream.stream_type, _duck, pmt.descs);

        // Process "elementary stream info" list of descriptors.
        analyzeDescriptors(stream.descs, svp.get(), ps.get());
    }
}


//----------------------------------------------------------------------------
// Analyze a NIT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeNIT(PID pid, const NIT& nit)
{
    PIDContextPtr ps(getPID(pid));

    // Document unreferenced NIT PID's.
    if (ps->description.empty() || ps->description == UNREFERENCED) {
        ps->description = u"NIT";
    }

    // Search network name. If not present, desc.name is empty.
    NetworkNameDescriptor desc;
    nit.descs.search(_duck, DID_DVB_NETWORK_NAME, desc);

    // Format network description as attribute of PID.
    ps->addAttribute(UString::Format(u"Network: %n %s", nit.network_id, desc.name).toTrimmed());

    // Collect information from LCN descriptors of different flavors.
    _lcn.addFromNIT(nit, _ts_id.value_or(0xFFFF));
}


//----------------------------------------------------------------------------
// Analyze an SDT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeSDT(const SDT& sdt)
{
    // Register characteristics of all services
    for (auto& it : sdt.services) {
        ServiceContextPtr svp(getService(it.first)); // it->first = map key = service id
        svp->orig_netw_id = sdt.onetw_id;
        svp->update(_duck, it.second.descs);
    }
}


//----------------------------------------------------------------------------
// Analyze a TDT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeTDT(const TDT& tdt)
{
    // Keep first and last time stamps
    if (_first_tdt == Time::Epoch) {
        _first_tdt = tdt.utc_time;
    }
    _last_tdt = tdt.utc_time;
}


//----------------------------------------------------------------------------
// Analyze a TOT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeTOT(const TOT& tot)
{
    // Keep first and last time stamps, country code of first region
    if (!tot.regions.empty()) {
        _last_tot = tot.localTime(tot.regions[0]);
        if (_first_tot == Time::Epoch) {
            _country_code = tot.regions[0].country;
            _first_tot = _last_tot;
        }
    }
}


//----------------------------------------------------------------------------
// Analyze an ATSC MGT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeMGT(const MGT& mgt)
{
    // Process all table types.
    for (auto& it : mgt.tables) {

        // The table type and its name.
        const MGT::TableType& tab(it.second);
        const UString name(u"ATSC " + MGT::TableTypeEnum().name(tab.table_type));

        // Get the PID context.
        const PIDContextPtr ps(getPID(tab.table_type_PID, name));
        ps->referenced = true;
        ps->carry_section = true;

        // An ATSC PID may carry more than one table type.
        if (ps->description != name) {
            ps->addAttribute(name);
        }

        // Some additional PSIP PID's shall be analyzed.
        switch (tab.table_type) {
            case ATSC_TTYPE_TVCT_CURRENT:
            case ATSC_TTYPE_CVCT_CURRENT:
                _demux.addPID(tab.table_type_PID);
                break;
            default:
                break;
        }
    }
}


//----------------------------------------------------------------------------
// Analyze an ATSC TVCT (terrestrial) or CVCT (cable)
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeVCT(const VCT& vct)
{
    // Register characteristics of all services
    for (auto& it : vct.channels) {
        const VCT::Channel& chan(it.second);

        // Only keep services from this transport stream.
        if (chan.channel_TSID == vct.transport_stream_id) {
            // Get or create the service with this service id ("program number" in ATSC parlance).
            ServiceContextPtr svp(getService(chan.program_number));
            const UString name(chan.short_name.toTrimmed());
            if (!name.empty()) {
                // Update the service name.
                svp->name = name;
            }
            // Provider is a DVB concept, we replace it with major.minor with ATSC.
            if (svp->provider.empty()) {
                svp->provider = UString::Format(u"ATSC %d.%d", chan.major_channel_number, chan.minor_channel_number);
            }
            svp->hidden = chan.hidden;
        }
    }
}


//----------------------------------------------------------------------------
// Analyze an ATSC STT.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeSTT(const STT& stt)
{
    // Keep first and last time stamps
    _last_stt = stt.utcTime();
    if (_first_stt == Time::Epoch) {
        _first_stt = _last_stt;
    }
}


//----------------------------------------------------------------------------
// Analyze an ISDB DCT.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeDCT(const DCT& dct)
{
    if (!_ts_id.has_value()) {
        // TS id is currently unknown, don't know where to look in DCT. Store it for later analysis.
        _dct = dct;
    }
    else {
        // Only look for current TS id.
        for (const auto& str : dct.streams) {
            if (str.transport_stream_id == _ts_id) {
                if (str.DL_PID != PID_NULL) {
                    const PIDContextPtr ps(getPID(str.DL_PID));
                    ps->addDescriptionOrAttribute(UString::Format(u"ISDB download (DLT)"));
                    ps->referenced = true;
                    ps->carry_section = true;
                    _demux.addPID(str.DL_PID);
                }
                if (str.ECM_PID != PID_NULL) {
                    const PIDContextPtr ps(getPID(str.ECM_PID));
                    ps->addDescriptionOrAttribute(UString::Format(u"ECM for ISDB download (DLT scrambling)"));
                    ps->referenced = true;
                    ps->carry_section = true;
                    _demux.addPID(str.ECM_PID);
                }
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Analyze an Astra-defined SGT (Service Guide Table).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeSGT(const SGT& sgt, PID pid)
{
    // The SGT defines Logical Channel Numbers (LCN).
    PIDContextPtr ps(getPID(pid));
    ps->description = u"Astra SGT";
    _lcn.addFromSGT(sgt, _ts_id.value_or(0xFFFF));
}


//----------------------------------------------------------------------------
// Return a full description, with comment and optionally attributes
//----------------------------------------------------------------------------

ts::UString ts::TSAnalyzer::PIDContext::fullDescription(bool include_attributes) const
{
    // Additional description
    UStringVector lines(languages);
    lines.push_back(comment);
    if (include_attributes) {
        lines.insert(lines.end(), attributes.begin(), attributes.end());
    }
    UString more(UString::Join(lines, u", ", true));

    // Return full description
    if (description.empty()) {
        return more;
    }
    else if (more.empty()) {
        return description;
    }
    else {
        return description + u" (" + more + u")";
    }
}


//----------------------------------------------------------------------------
// Analyse a list of descriptors.
// If svp is not 0, we are in the PMT of the specified service.
// If ps is not 0, we are in the description of this PID in a PMT.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeDescriptors(const DescriptorList& descs, ServiceContext* svp, PIDContext* ps)
{
    for (size_t di = 0; di < descs.count(); ++di) {

        const Descriptor& bindesc(descs[di]);
        const uint8_t* data = bindesc.payload();
        size_t size = bindesc.payloadSize();

        switch (bindesc.tag()) {
            case DID_MPEG_CA: {
                // MPEG standard CA descriptor.
                analyzeCADescriptor(bindesc, svp, ps);
                break;
            }
            case DID_ISDB_CA:
            case DID_ISDB_COND_PLAYBACK: {
                // ISDB specific CA descriptors.
                if (bool(_duck.standards() & Standards::ISDB)) {
                    analyzeCADescriptor(bindesc, svp, ps, u" (ISDB)");
                }
                break;
            }
            case DID_MPEG_LANGUAGE: {
                if (ps != nullptr) {
                    const ISO639LanguageDescriptor desc(_duck, bindesc);
                    for (auto& e : desc.entries) {
                        AppendUnique(ps->languages, e.language_code);
                        if (e.audio_type != 0) {
                            ps->comment = e.audioTypeName();
                        }
                    }
                }
                break;
            }
            case DID_DVB_AC3: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an AC-3 audio track.
                    ps->description = u"AC-3 Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_DVB_ENHANCED_AC3: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an Enhanced AC-3 audio track.
                    ps->description = u"E-AC-3 Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_DVB_AAC: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an AAC, E-AAC or HE-AAC audio track.
                    const AACDescriptor desc(_duck, bindesc);
                    const UString type(desc.aacTypeString());
                    if (!type.empty()) {
                        ps->description = type;
                    }
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_DVB_DTS: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates a DTS audio track.
                    ps->description = u"DTS Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_MPEG_VVC_VIDEO: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates a VVC video track.
                    ps->description = u"VVC Video";
                    ps->carry_video = true;
                }
                break;
            }
            case DID_MPEG_EVC_VIDEO: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an EVC video track.
                    ps->description = u"EVC Video";
                    ps->carry_video = true;
                }
                break;
            }
            case DID_DVB_SUBTITLING: {
                if (ps != nullptr) {
                    ps->description = u"Subtitles";
                    const SubtitlingDescriptor desc(_duck, bindesc);
                    for (auto& e : desc.entries) {
                        AppendUnique(ps->languages, e.language_code);
                        ps->addAttribute(e.subtitlingTypeName());
                    }
                }
                break;
            }
            case DID_DVB_TELETEXT: {
                if (ps != nullptr) {
                    ps->description = u"Teletext";
                    const TeletextDescriptor desc(_duck, bindesc);
                    for (auto& e : desc.entries) {
                        AppendUnique(ps->languages, e.language_code);
                        ps->addAttribute(NameFromSection(u"dtv", u"teletext_descriptor.teletext_type", e.teletext_type));
                    }
                }
                break;
            }
            case DID_DVB_APPLI_SIGNALLING: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates a PID carrying an AIT.
                    ps->comment = u"AIT";
                }
                break;
            }
            case DID_MPEG_REGISTRATION: {
                if (ps != nullptr) {
                    const RegistrationDescriptor desc(_duck, bindesc);
                    switch (desc.format_identifier) {
                        case REGID_BSSD: {
                            // The presence of this registration id indicates an AES3 PCM audio track (SMPTE 302M).
                            ps->description = u"AES3 PCM Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        case REGID_VC1: {
                            ps->description = u"VC-1 Video";
                            ps->carry_video = true;
                            break;
                        }
                        case REGID_VC4: {
                            ps->description = u"VC-4 Video";
                            ps->carry_video = true;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                break;
            }
            case DID_MPEG_EXTENSION: {
                // MPEG extension descriptor: need to look at the descriptor_tag_extension.
                if (size >= 1) {
                    switch (data[0]) {
                        case XDID_MPEG_LCEVC_VIDEO: {
                            // The presence of this descriptor indicates an LCEVC video track.
                            ps->description = u"LCEVC Video";
                            ps->carry_video = true;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                break;
            }
            case DID_DVB_EXTENSION: {
                // Extension descriptor: need to look at the descriptor_tag_extension.
                if (size >= 1) {
                    switch (data[0]) {
                        case XDID_DVB_AC4: {
                            // The presence of this descriptor indicates an AC-4 audio track.
                            ps->description = u"AC-4 Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        case XDID_DVB_DTS_HD_AUDIO: {
                            // The presence of this descriptor indicates an DTS-HD audio track.
                            ps->description = u"DTS-HD Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        case XDID_DVB_DTS_NEURAL: {
                            // The presence of this descriptor indicates an DTS-Neural audio track.
                            ps->description = u"DTS Neural Surround Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                break;
            }
            case DID_DVB_DATA_BROADCAST_ID: {
                if (size >= 2) {
                    // Get the data broadcast id.
                    const uint16_t dbid = GetUInt16(data);
                    switch (dbid) {
                        case 0x000A: {
                            // System Software Update(SSU, ETSI TS 102 006)
                            // Skip data_broadcast_id, already checked == 0x000A
                            data += 2; size -= 2;
                            if (svp != nullptr) {
                                // Mark the service as carrying SSU
                                svp->carry_ssu = true;
                            }
                            if (ps != nullptr && size >= 1) {
                                // Rest of descriptor is a system_software_update_info structure.
                                // Store the list of OUI's in PID context.
                                // OUI_data_length:
                                uint8_t dlength = data[0];
                                data += 1; size -= 1;
                                if (dlength > size) {
                                    dlength = uint8_t(size);
                                }
                                // OUI loop:
                                while (dlength >= 6) {
                                    // Fixed part (6 bytes) followed by variable-length selector
                                    uint32_t oui = GetUInt32(data - 1) & 0x00FFFFFF; // 24 bits
                                    uint8_t slength = data[5];
                                    data += 6; size -= 6; dlength -= 6;
                                    if (slength > dlength) {
                                        slength = dlength;
                                    }
                                    data += slength; size -= slength; dlength -= slength;
                                    // Store OUI in PID context
                                    ps->ssu_oui.insert(oui);
                                }
                            }
                            break;
                        }
                        case 0x0005: {
                            // Multi-Protocol Encapsulation.
                            if (ps != nullptr) {
                                ps->comment = u"MPE";
                            }
                            break;
                        }
                        case 0x000B: {
                            // IP/MAC Notification Table.
                            if (ps != nullptr) {
                                ps->comment = u"INT";
                            }
                            break;
                        }
                        case 0x0123: {
                            // HbbTV data carousel.
                            if (ps != nullptr) {
                                ps->comment = u"HbbTV";
                            }
                            break;
                        }
                        default: {
                            if (ps != nullptr) {
                                ps->comment = DataBroadcastIdName(dbid);
                            }
                            break;
                        }
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Analyse one CA descriptor, either from the CAT or a PMT.
//  If svp is not 0, we are in the PMT of the specified service.
//  If ps is not 0, we are in the description of this PID in a PMT.
//  If svp is 0, we are in the CAT.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeCADescriptor(const Descriptor& desc, ServiceContext* svp, PIDContext* ps, const UString& suffix)
{
    const uint8_t* data(desc.payload());
    size_t size(desc.payloadSize());

    // Analyze the common part
    if (size < 4) {
        return;
    }
    const uint16_t ca_sysid = GetUInt16(data);
    const CASFamily cas = CASFamilyOf(ca_sysid);
    const PID ca_pid = GetUInt16(data + 2) & 0x1FFF;
    data += 4; size -= 4;

    // On ISDB streams, we sometimes see the NULL PID as CA PID.
    if (ca_pid == PID_NULL) {
        return;
    }

    // Process CA descriptor private data
    if (cas == CAS_MEDIAGUARD && svp != nullptr && size >= 13) {

        // MediaGuard CA descriptor in a PMT
        data -= 2; size += 2;
        while (size >= 15) {
            PID pid(GetUInt16(data) & 0x1FFF);
            uint16_t opi(GetUInt16(data + 2));
            // Found an ECM PID for the service
            PIDContextPtr eps(getPID(pid));
            eps->addService(svp->service_id);
            eps->carry_ecm = true;
            eps->cas_id = ca_sysid;
            eps->cas_operators.insert(opi);
            eps->carry_section = true;
            _demux.addPID(ca_pid);
            eps->description.format(u"MediaGuard ECM for OPI %n", opi);
            data += 15; size -= 15;
        }
    }

    else if (cas == CAS_MEDIAGUARD && svp == nullptr && size == 4) {

        // MediaGuard CA descriptor in the CAT, new format
        uint16_t etypes(GetUInt16(data));
        uint16_t opi(GetUInt16(data + 2));
        PIDContextPtr eps(getPID(ca_pid));
        eps->referenced = true;
        eps->carry_emm = true;
        eps->cas_id = ca_sysid;
        eps->cas_operators.insert(opi);
        eps->carry_section = true;
        _demux.addPID(ca_pid);
        eps->description.format(u"MediaGuard EMM for OPI %n, EMM types: 0x%X", opi, etypes);
    }

    else if (cas == CAS_MEDIAGUARD && svp == nullptr && size >= 1) {

        // MediaGuard CA descriptor in the CAT, old format
        uint8_t nb_opi = data[0];
        data++; size --;
        PIDContextPtr eps(getPID(ca_pid));
        eps->referenced = true;
        eps->carry_emm = true;
        eps->cas_id = ca_sysid;
        eps->carry_section = true;
        _demux.addPID(ca_pid);
        eps->description = u"MediaGuard Individual EMM";

        while (nb_opi > 0 && size >= 4) {
            PID pid(GetUInt16(data) & 0x1FFF);
            uint16_t opi(GetUInt16(data + 2));
            PIDContextPtr eps1(getPID(pid));
            eps1->referenced = true;
            eps1->carry_emm = true;
            eps1->cas_id = ca_sysid;
            eps1->cas_operators.insert(opi);
            eps1->carry_section = true;
            _demux.addPID(ca_pid);
            eps1->description = UString::Format(u"MediaGuard Group EMM for OPI %n", opi);
            data += 4; size -= 4; nb_opi--;
        }
    }

    else if (cas == CAS_SAFEACCESS && svp == nullptr && size >= 1) {

        // SafeAccess CA descriptor in the CAT
        data++; size --; // skip applicable EMM bitmask
        PIDContextPtr eps(getPID(ca_pid));
        eps->referenced = true;
        eps->carry_emm = true;
        eps->cas_id = ca_sysid;
        eps->carry_section = true;
        _demux.addPID(ca_pid);
        eps->description = u"SafeAccess EMM";

        while (size >= 2) {
            uint16_t ppid = GetUInt16(data);
            data += 2; size -= 2;
            if (eps->cas_operators.empty()) {
                eps->description += UString::Format(u" for PPID %n", ppid);
            }
            else {
                eps->description += UString::Format(u", %n", ppid);
            }
            eps->cas_operators.insert(ppid);
        }
    }

    else if (cas == CAS_VIACCESS) {

        // Viaccess CA descriptor in the CAT or PMT
        PIDContextPtr eps(getPID(ca_pid));
        eps->referenced = true;
        eps->cas_id = ca_sysid;
        eps->carry_section = true;
        _demux.addPID(ca_pid);

        if (svp == nullptr) {
            // No service, this is an EMM PID
            eps->carry_emm = true;
            eps->description = u"Viaccess EMM";
        }
        else {
            // Found an ECM PID for the service
            eps->carry_ecm = true;
            eps->addService(svp->service_id);
            eps->description = u"Viaccess ECM";
        }

        while (size >= 2) {
            const uint8_t tag = data[0];
            size_t len = data[1];
            data += 2; size -= 2;
            if (len > size) {
                len = size;
            }
            if (tag == 0x14 && len == 3) {
                const uint32_t soid = GetUInt24(data);
                if (eps->cas_operators.empty()) {
                    eps->description += UString::Format(u" for SOID %d (0x%06X)", soid, soid);
                }
                else {
                    eps->description += UString::Format(u", %d (0x%06X)", soid, soid);
                }
                eps->cas_operators.insert(soid);
            }
            data += len; size -= len;
        }
    }

    else {

        // Other CA descriptor, general format
        PIDContextPtr eps(getPID(ca_pid));
        eps->referenced = true;
        eps->cas_id = ca_sysid;
        eps->carry_section = true;
        _demux.addPID(ca_pid);

        if (svp == nullptr) {
            // No service, this is an EMM PID
            eps->carry_emm = true;
            eps->description = CASIdName(_duck, ca_sysid) + u" EMM" + suffix;
        }
        else {
            // Found an ECM PID for the service
            eps->carry_ecm = true;
            eps->addService(svp->service_id);
            eps->description = CASIdName(_duck, ca_sysid) + u" ECM" + suffix;
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new audio attributes are found in an audio PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket& pkt, const MPEG2AudioAttributes& attr)
{
    PIDContextPtr ps(getPID(pkt.sourcePID()));

    // AAC audio streams have the same outer syntax and are sometimes incorrectly reported as MPEG-2 audio.
    if (ps->stream_type == ST_MPEG1_AUDIO || ps->stream_type == ST_MPEG2_AUDIO) {
        // We are sure that the stream is MPEG 1/2 Audio.
        ps->addAttribute(attr.toString());
    }
    else if (ps->stream_type == ST_NULL) {
        // We do not know the stream type yet, the first PES packet came before the PMT.
        ps->audio2 = attr;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an invalid PES packet is received.
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleInvalidPESPacket(PESDemux&, const DemuxedData& data)
{
    getPID(data.sourcePID())->inv_pes++;
}


//----------------------------------------------------------------------------
// This hook is invoked when new AC-3 attributes are found in an audio PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewAC3Attributes(PESDemux&, const PESPacket& pkt, const AC3Attributes& attr)
{
    getPID(pkt.sourcePID())->addAttribute(attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new video attributes are found in a video PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket& pkt, const MPEG2VideoAttributes& attr)
{
    getPID(pkt.sourcePID())->addAttribute(attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new AVC attributes are found in a video PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewAVCAttributes(PESDemux&, const PESPacket& pkt, const AVCAttributes& attr)
{
    getPID(pkt.sourcePID())->addAttribute(attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new HEVC attributes are found in a video PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewHEVCAttributes(PESDemux&, const PESPacket& pkt, const HEVCAttributes& attr)
{
    getPID(pkt.sourcePID())->addAttribute(attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when a new PID carrying T2-MI is available.
// (Implementation of T2MIHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc)
{
    // Identify this service as T2-MI, if not yet identified.
    ServiceContextPtr svp(getService(pmt.service_id));
    svp->carry_t2mi = true;
    if (svp->name.empty()) {
        svp->name = u"(T2-MI)";
    }

    // Identify this PID as T2-MI, if not yet identified.
    PIDContextPtr pc(getPID(pid));
    pc->description = u"T2-MI";
    pc->carry_t2mi = true;
    pc->carry_section = false;

    // And demux all T2-MI packets.
    _t2mi_demux.addPID(pid);
}


//----------------------------------------------------------------------------
// This hook is invoked when a new T2-MI packet is available.
// (Implementation of T2MIHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt)
{
    PIDContextPtr pc(getPID(pkt.sourcePID(), u"T2-MI"));

    // Count T2-MI packets.
    pc->t2mi_cnt++;

    // Process PLP (only in baseband frame).
    if (pkt.plpValid()) {
        // Make sure the PLP is referenced, even if no TS packet is demux'ed.
        pc->t2mi_plp_ts[pkt.plp()];

        // Add the PLP as attributes of this PID.
        pc->addAttribute(UString::Format(u"PLP: %n", pkt.plp()));
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a new TS packet is extracted from T2-MI.
// (Implementation of T2MIHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts)
{
    PIDContextPtr pc(getPID(t2mi.sourcePID(), u"T2-MI"));

    // Count demux'ed TS packets from this PLP.
    pc->t2mi_plp_ts[t2mi.plp()]++;
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata)
{
    bool broken_rate(false);

    // Store system times of first packet
    if (_first_utc == Time::Epoch) {
        _first_utc = Time::CurrentUTC();
        _first_local = Time::CurrentLocalTime();
    }

    // Each new packet leads to various modifications
    _modified = true;

    // Count TS packets
    _ts_pkt_cnt++;
    uint64_t packet_index(_ts_pkt_cnt);

    // Detect and ignore invalid packets
    bool invalid_packet = false;
    if (!pkt.hasValidSync()) {
        _invalid_sync++;
        invalid_packet = true;
    }
    if (pkt.getTEI()) {
        _transport_errors++;
        invalid_packet = true;
    }
    if (invalid_packet) {
        _preceding_errors++;
        _preceding_suspects = 0;
        return;
    }

    // Detect and ignore suspect packets
    if (_min_error_before_suspect > 0 && _max_consecutive_suspects > 0 && !pidExists(pkt.getPID())) {
        // Suspect packet detection enabled and potential suspect packet
        if (_preceding_errors >= _min_error_before_suspect || (_preceding_suspects > 0 && _preceding_suspects < _max_consecutive_suspects)) {
            _suspect_ignored++;
            _preceding_suspects++;
            _preceding_errors = 0;
            return;
        }
    }

    // Packet is not suspect, reset suspect detection
    _preceding_errors = 0;
    _preceding_suspects = 0;

    // Feed packets into the various demux
    _demux.feedPacket(pkt);
    _pes_demux.feedPacket(pkt);
    _t2mi_demux.feedPacket(pkt);

    // Get PID context
    PIDContextPtr ps(getPID(pkt.getPID()));
    ps->ts_pkt_cnt++;

    // Accumulate stat from packet
    if (pkt.hasAF()) {
        ps->ts_af_cnt++;
    }
    if (pkt.getPUSI()) {
        ps->unit_start_cnt++;
    }
    if (pkt.getPUSI() && pkt.hasPayload()) {
        ps->pl_start_cnt++;
    }

    // Process scrambling information
    if (pkt.getScrambling() != SC_CLEAR && !ps->scrambled) {
        ps->scrambled = true;
        _scrambled_pid_cnt++;
    }
    if (pkt.getScrambling() == SC_DVB_RESERVED) {
        ps->inv_ts_sc_cnt++;
    }
    else if (pkt.getScrambling() != SC_CLEAR) {
        ps->ts_sc_cnt++;
    }
    if (pkt.getScrambling() != ps->cur_ts_sc) {
        // Change of crypto-period
        if (ps->cur_ts_sc != SC_CLEAR) {
            // End of a crypto-period, not a clear/scramble transition.
            // Count number of crypto-periods:
            ps->cryptop_cnt++;
            // Count number of TS packets in all crypto-periods.
            // Ignore first crypto-period since it is truncated and
            // not significant for evaluation of duration.
            if (ps->cryptop_cnt > 1) {
                ps->cryptop_ts_cnt += packet_index - ps->cur_ts_sc_pkt;
            }
        }
        ps->cur_ts_sc = pkt.getScrambling();
        ps->cur_ts_sc_pkt = packet_index;
    }

    // PID_IIP (0x1FF0) is a global PID with ISDB.
    if (ps->pid == PID_IIP && !ps->carry_iip && bool(_duck.standards() & Standards::ISDB) && ps->services.empty()) {
        // First time we can consider this PID as IIP. Can be first packet in the PID and we knwow that we use ISDB
        // or not first packet in the PID but we didn(t know yet the TS was ISDB.
        ps->carry_iip = true;
        ps->referenced = true;
        ps->description = u"ISDB IIP";
    }

    // Process discontinuities.
    // The continuity counter of null packets is undefined.
    if (ps->pid != PID_NULL) {
        if (ps->ts_pkt_cnt == 1) {
            // First packet, initialize continuity
            ps->cur_continuity = pkt.getCC();
        }
        else if (pkt.getDiscontinuityIndicator()) {
            // Expected discontinuity
            ps->exp_discont++;
            broken_rate = true;
        }
        else if (pkt.hasPayload()) {
            // Packet has payload.
            if (pkt.getCC() == ps->cur_continuity) {
                // Same counter means duplicated packet.
                ps->duplicated++;
            }
            else if (pkt.getCC() != (ps->cur_continuity + 1) % CC_MAX) {
                // Counter not following previous -> discontinuity
                ps->unexp_discont++;
                broken_rate = true;
            }
        }
        else if (pkt.getCC() != ps->cur_continuity) {
            // Packet has no payload -> should have same counter
            ps->unexp_discont++;
            broken_rate = true;
        }
        ps->cur_continuity = pkt.getCC();
    }

    // Process clocks.
    const uint64_t pcr = pkt.getPCR();
    const uint64_t pts = pkt.getPTS();
    const uint64_t dts = pkt.getDTS();
    if (broken_rate) {
        // Suspected packet loss, forget the last PCR with use to compute bitrate.
        ps->br_last_pcr = INVALID_PCR;
    }
    if (pcr != INVALID_PCR) {
        // Count PID's with PCR
        if (ps->pcr_cnt++ == 0) {
            _pcr_pid_cnt++;
        }
        // If last PCR valid, compute transport rate between the two
        if (ps->br_last_pcr != INVALID_PCR && ps->br_last_pcr < pcr) {
            // Compute transport rate in b/s since last PCR
            BitRate ts_bitrate = BitRate((packet_index - ps->br_last_pcr_pkt) * SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS) / (pcr - ps->br_last_pcr);
            // Per-PID statistics:
            ps->ts_bitrate_sum += ts_bitrate;
            ps->ts_bitrate_cnt++;
            // Transport stream statistics:
            _ts_bitrate_sum += ts_bitrate;
            _ts_bitrate_cnt++;
        }
        // Detect PCR leaps.
        if (ps->last_pcr != INVALID_PCR && (ps->last_pcr > pcr || (pcr - ps->last_pcr) > SYSTEM_CLOCK_FREQ)) {
            // PCR wrap-up or more than one second diff.
            ps->pcr_leap_cnt++;
        }
        // Save PCR for next calculation
        ps->br_last_pcr = pcr;
        ps->br_last_pcr_pkt = packet_index;
        // Save first and last PCR outside of bitrate computation.
        if (ps->first_pcr == INVALID_PCR) {
            ps->first_pcr = pcr;
        }
        ps->last_pcr = pcr;
    }
    if (pts != INVALID_PTS) {
        ps->pts_cnt++;
        if (ps->last_pts != INVALID_PTS) {
            // PTS are allowed to be out-of-order.
            const uint64_t diff = pts > ps->last_pts ? pts - ps->last_pts : ps->last_pts - pts;
            if (diff > 3 * SYSTEM_CLOCK_SUBFREQ) {
                // PTS wrap-up or more than 3 seconds diff.
                ps->pts_leap_cnt++;
            }
        }
        if (ps->first_pts == INVALID_PTS) {
            ps->first_pts = pts;
        }
        ps->last_pts = pts;
    }
    if (dts != INVALID_DTS) {
        ps->dts_cnt++;
        if (ps->last_dts != INVALID_DTS && (ps->last_dts > dts || (dts - ps->last_dts) > 3 * SYSTEM_CLOCK_SUBFREQ)) {
            // DTS wrap-up or more than 3 seconds diff.
            ps->dts_leap_cnt++;
        }
        if (ps->first_dts == INVALID_DTS) {
            ps->first_dts = dts;
        }
        ps->last_dts = dts;
    }

    // Check PES start code: PES packet headers start with the constant sequence 00 00 01.
    // Check this on all clear packets. This test is actually meaningful only on TS packets carrying PES packets.
    // Note that "carrying PES" is an information that is not available from the packet itself but from the environment
    // (for instance if the PID is referenced as a video PID in a PMT). So, before getting the PMT referencing a PID,
    // we do not know if this PID carries PES or not.
    size_t header_size = pkt.getHeaderSize();
    if (pkt.getPUSI() && pkt.getScrambling() == SC_CLEAR && header_size <= PKT_SIZE - 3) {

        // Got a "unit start indicator" in a clear packet.
        // This may be the start of a section or a PES packet.

        if (pkt.b[header_size] != 0x00 || pkt.b[header_size + 1] != 0x00 || pkt.b[header_size + 2] != 0x01) {
            // Got an invalid PES start code. This is not an error if the
            // PID carries sections (we may not yet know this, so count
            // all these errors now and ignore them later if we know
            // that the PID does not carry PES packets).
            ps->inv_pes_start++;
        }
        else if (header_size <= PKT_SIZE - 4 && ps->pid != 0) {
            // Here, the start of the packet payload is 00 00 01.
            // The only case where this can happen on a section is a PAT
            // (first 00 = "pointer field", second 00 = table_id = PAT).
            // A PAT is normally available on PID 0 only. Since we have
            // excluded PID 0 in the test above, this cannot be a PAT.
            // As a consequence, we are pretty sure to have a PES packet.
            // Remember the stream_id of the PES packets on this PID
            // (the PES stream_id is next byte after PES start code).
            if (ps->pes_stream_id == 0) {
                // First PES stream_id found on this PID
                ps->pes_stream_id = pkt.b[header_size + 3];
                ps->same_stream_id = true;
            }
            else if (ps->pes_stream_id != pkt.b[header_size + 3]) {
                // Got different values of stream_id in PES packets
                ps->same_stream_id = false;
            }
        }
    }

    // Check "ISDB-T information" in extended 16-byte trailer. The 16-byte trailer is only available when
    // analyzing transport streams with 204-byte packets. In that case, the trailer is in the packet
    // metadata. At this point, we don't always know if the stream is an ISDB one or not. We collect
    // the information as if the TS was ISDB. At reporting time, we will use it only if the stream is
    // confirmed as ISDB.
    ISDBTInformation info(_duck, mdata, false);
    if (info.is_valid) {
        // Count packets in the ISDB-T layers. Some PID's have all their packets in the same layers.
        // Some other PID's have been seen on multiple layers.
        ps->isdb_layers[info.layer_indicator]++;
    }
}


//----------------------------------------------------------------------------
// Specify a "bitrate hint" for the analysis. It is the user-specified
// bitrate in bits/seconds, based on 188-byte packets. The bitrate is
// optional: if specified as zero, the analysis is based on the PCR values.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::setBitrateHint(const BitRate& bitrate_hint, BitRateConfidence bitrate_confidence)
{
    _ts_user_bitrate = bitrate_hint;
    _ts_user_br_confidence = bitrate_confidence;
    _modified = true;
}


//----------------------------------------------------------------------------
// Update the global statistics value if internal data were modified.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::recomputeStatistics()
{
    // Don't do anything if not necessary
    if (!_modified) {
        return;
    }

    // Store "last" system times.
    _last_utc = Time::CurrentUTC();
    _last_local = Time::CurrentLocalTime();

    // Select the reference bitrate from the user-specified and PCR-evaluated values
    // based on their respective confidences.
    _ts_pcr_bitrate_188 = _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_sum / _ts_bitrate_cnt);
    _ts_pcr_bitrate_204 = _ts_bitrate_cnt == 0 ? 0 : BitRate((_ts_bitrate_sum * PKT_RS_SIZE) / (_ts_bitrate_cnt * PKT_SIZE));
    _ts_bitrate = SelectBitrate(_ts_user_bitrate, _ts_user_br_confidence, _ts_pcr_bitrate_188, BitRateConfidence::PCR_AVERAGE);

    // Compute broadcast duration.
    _duration = PacketInterval(_ts_bitrate, _ts_pkt_cnt);

    // Reinitialize all service information that will be updated PID by PID
    for (auto& srv : _services) {
        srv.second->pid_cnt = 0;
        srv.second->ts_pkt_cnt = 0;
        srv.second->scrambled_pid_cnt = 0;
        srv.second->isdb_layers.clear();
    }

    // Shall we use ISDB information?
    const bool isdb = bool(_duck.standards() & Standards::ISDB);

    // Complete all PID information
    _pid_cnt = 0;
    _global_pid_cnt = 0;
    _global_pkt_cnt = 0;
    _global_scr_pids = 0;
    _psisi_pid_cnt = 0;
    _psisi_pkt_cnt = 0;
    _psisi_scr_pids = 0;
    _unref_pid_cnt = 0;
    _unref_pkt_cnt = 0;
    _unref_scr_pids = 0;
    _ts_isdb_layers.clear();
    _global_isdb_layers.clear();
    _unref_isdb_layers.clear();

    for (auto& pci : _pids) {
        PIDContext& pc(*pci.second);

        // Count total packets.
        if (isdb) {
            _ts_isdb_layers.accumulate(pc.isdb_layers);
        }

        // Compute TS bitrate from the PCR's of this PID
        if (pc.ts_bitrate_cnt != 0) {
            pc.ts_pcr_bitrate = pc.ts_bitrate_sum / pc.ts_bitrate_cnt;
        }

        // Compute average PID bitrate
        if (_ts_pkt_cnt != 0) {
            pc.bitrate = (_ts_bitrate * pc.ts_pkt_cnt) / _ts_pkt_cnt;
        }

        // Compute average crypto-period for this PID
        // Remember that first crypto-period was ignored.
        if (pc.cryptop_cnt > 1) {
            pc.crypto_period = pc.cryptop_ts_cnt / (pc.cryptop_cnt - 1);
        }

        // If the PID belongs to some services, update services info.
        for (auto& it : pc.services) {
            ServiceContextPtr scp(getService(it));
            scp->pid_cnt++;
            scp->ts_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                scp->scrambled_pid_cnt++;
            }
            if (isdb) {
                scp->isdb_layers.accumulate(pc.isdb_layers);
            }
        }

        // Enforce PES when carrying audio or video
        pc.carry_pes = pc.carry_pes || pc.carry_audio || pc.carry_video;

        // Count non-empty PID's
        if (pc.ts_pkt_cnt != 0) {
            _pid_cnt++;
        }

        // Count unreferenced PID's
        if (!pc.referenced && pc.ts_pkt_cnt != 0) {
            _unref_pid_cnt++;
            _unref_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                _unref_scr_pids++;
            }
            if (isdb) {
                _unref_isdb_layers.accumulate(pc.isdb_layers);
            }
        }

        // Count global PID's
        if (pc.referenced && pc.services.size() == 0 && pc.ts_pkt_cnt != 0) {
            _global_pid_cnt++;
            _global_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                _global_scr_pids++;
            }
            if (isdb) {
                _global_isdb_layers.accumulate(pc.isdb_layers);
            }
        }

        // Count global PSI/SI PID's
        if (pc.pid <= PID_DVB_LAST && pc.services.size() == 0 && pc.ts_pkt_cnt != 0) {
            _psisi_pid_cnt++;
            _psisi_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                _psisi_scr_pids++;
            }
        }
    }

    // Complete unreferenced and global PID's bitrates
    if (_ts_pkt_cnt != 0) {
        _global_bitrate = (_ts_bitrate * _global_pkt_cnt) / _ts_pkt_cnt;
        _psisi_bitrate = (_ts_bitrate * _psisi_pkt_cnt) / _ts_pkt_cnt;
        _unref_bitrate = (_ts_bitrate * _unref_pkt_cnt) / _ts_pkt_cnt;
    }

    // Complete all service information
    _scrambled_services_cnt = 0;

    for (auto& sv : _services) {

        // Count scrambled services
        if (sv.second->scrambled_pid_cnt > 0) {
            _scrambled_services_cnt++;
        }

        // Compute average service bitrate
        if (_ts_pkt_cnt == 0) {
            sv.second->bitrate = 0;
        }
        else {
            sv.second->bitrate = (_ts_bitrate * sv.second->ts_pkt_cnt) / _ts_pkt_cnt;
        }

        // Collect info from LCN descriptors.
        const uint16_t lcn = _lcn.getLCN(sv.first, _ts_id.value_or(0xFFFF), sv.second->orig_netw_id.value_or(0xFFFF));
        if (lcn != 0xFFFF) {
            sv.second->lcn = lcn;
        }
        if (!sv.second->hidden) {
            sv.second->hidden = !_lcn.getVisible(sv.first, _ts_id.value_or(0xFFFF), sv.second->orig_netw_id.value_or(0xFFFF));
        }
    }

    // Don't redo this unless the analyzer is modified
    _modified = false;
}


//----------------------------------------------------------------------------
// Return the list of service ids
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getServiceIds(std::vector<uint16_t>& list)
{
    recomputeStatistics();
    list.clear();

    for (auto& srv : _services) {
        list.push_back(srv.first);
    }
}


//----------------------------------------------------------------------------
// Return the list of PIDs
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getPIDs(std::vector<PID>& list)
{
    recomputeStatistics();
    list.clear();

    for (auto& pid : _pids) {
        if (pid.second->ts_pkt_cnt > 0) {
            list.push_back(pid.first);
        }
    }
}


//----------------------------------------------------------------------------
// Return the list of global PIDs
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getGlobalPIDs(std::vector<PID>& list)
{
    recomputeStatistics();
    list.clear();

    for (auto& pid : _pids) {
        if (pid.second->referenced && pid.second->services.empty() && pid.second->ts_pkt_cnt > 0) {
            list.push_back(pid.first);
        }
    }
}


//----------------------------------------------------------------------------
// Return the list of unreferenced PIDs
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getUnreferencedPIDs(std::vector<PID>& list)
{
    recomputeStatistics();
    list.clear();

    for (auto& pid : _pids) {
        if (!pid.second->referenced && pid.second->ts_pkt_cnt > 0) {
            list.push_back(pid.first);
        }
    }
}


//----------------------------------------------------------------------------
// Return the list of PIDs for one service id
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getPIDsOfService(std::vector<PID>& list, uint16_t service_id)
{
    recomputeStatistics();
    list.clear();

    for (auto& pid : _pids) {
        if (pid.second->services.count(service_id) > 0) {
            list.push_back(pid.first);
        }
    }
}


//----------------------------------------------------------------------------
// Return the list of PIDs carrying PES packets
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getPIDsWithPES(std::vector<PID>& list)
{
    recomputeStatistics();
    list.clear();

    for (auto& pid : _pids) {
        if (pid.second->carry_pes) {
            list.push_back(pid.first);
        }
    }
}

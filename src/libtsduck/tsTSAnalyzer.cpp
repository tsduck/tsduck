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
//  This class analyzes a complete transport stream.
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzer.h"
#include "tsT2MIPacket.h"
#include "tsNames.h"
#include "tsAlgorithm.h"
TSDUCK_SOURCE;

// Constant string "Unreferenced"
const ts::UString ts::TSAnalyzer::UNREFERENCED(u"Unreferenced");


//----------------------------------------------------------------------------
// Constructor for the TS analyzer
//----------------------------------------------------------------------------

ts::TSAnalyzer::TSAnalyzer(BitRate bitrate_hint) :
    _ts_id(0),
    _ts_id_valid(false),
    _ts_pkt_cnt(0),
    _invalid_sync(0),
    _transport_errors(0),
    _suspect_ignored(0),
    _pid_cnt(0),
    _scrambled_pid_cnt(0),
    _pcr_pid_cnt(0),
    _global_pid_cnt(0),
    _global_scr_pids(0),
    _global_pkt_cnt(0),
    _global_bitrate(0),
    _psisi_pid_cnt(0),
    _psisi_scr_pids(0),
    _psisi_pkt_cnt(0),
    _psisi_bitrate(0),
    _unref_pid_cnt(0),
    _unref_scr_pids(0),
    _unref_pkt_cnt(0),
    _unref_bitrate(0),
    _ts_pcr_bitrate_188(0),
    _ts_pcr_bitrate_204(0),
    _ts_user_bitrate(bitrate_hint),
    _ts_bitrate(0),
    _duration(0),
    _first_utc(Time::Epoch),
    _last_utc(Time::Epoch),
    _first_local(Time::Epoch),
    _last_local(Time::Epoch),
    _first_tdt(Time::Epoch),
    _last_tdt(Time::Epoch),
    _first_tot(Time::Epoch),
    _last_tot(Time::Epoch),
    _country_code(),
    _scrambled_services_cnt(0),
    _tid_present(),
    _pids(),
    _services(),
    _modified(false),
    _ts_bitrate_sum(0),
    _ts_bitrate_cnt(0),
    _preceding_errors(0),
    _preceding_suspects(0),
    _min_error_before_suspect(1),
    _max_consecutive_suspects(1),
    _default_charset(nullptr),
    _demux(this, this),
    _pes_demux(this),
    _t2mi_demux(this)
{
    // Specify the PID filters to collect PSI tables.
    // Start with all MPEG/DVB reserved PID's.
    for (PID pid = 0; pid <= PID_DVB_LAST; ++pid) {
        _demux.addPID(pid);
    }
}


//----------------------------------------------------------------------------
// Destructor for the TS analyzer
//----------------------------------------------------------------------------

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
    _ts_id = 0;
    _ts_id_valid = false;
    _ts_pkt_cnt = 0;
    _invalid_sync = 0;
    _transport_errors = 0;
    _suspect_ignored = 0;
    _pid_cnt = 0;
    _scrambled_pid_cnt = 0;
    _pcr_pid_cnt = 0;
    _global_pid_cnt = 0;
    _global_scr_pids = 0;
    _global_pkt_cnt = 0;
    _global_bitrate = 0;
    _psisi_pid_cnt = 0;
    _psisi_scr_pids = 0;
    _psisi_pkt_cnt = 0;
    _psisi_bitrate = 0;
    _unref_pid_cnt = 0;
    _unref_scr_pids = 0;
    _unref_pkt_cnt = 0;
    _unref_bitrate = 0;
    _ts_pcr_bitrate_188 = 0;
    _ts_pcr_bitrate_204 = 0;
    _ts_user_bitrate = 0;
    _ts_bitrate = 0;
    _duration = 0;
    _first_utc = Time::Epoch;
    _last_utc = Time::Epoch;
    _first_local = Time::Epoch;
    _last_local = Time::Epoch;
    _first_tdt = Time::Epoch;
    _last_tdt = Time::Epoch;
    _first_tot = Time::Epoch;
    _last_tot = Time::Epoch;
    _country_code.clear();
    _scrambled_services_cnt = 0;
    _tid_present.reset();
    _pids.clear();
    _services.clear();
    _ts_bitrate_sum = 0;
    _ts_bitrate_cnt = 0;
    _preceding_errors = 0;
    _preceding_suspects = 0;
    _demux.reset();
    _pes_demux.reset();

    // Specify the PID filters to collect PSI tables.
    // Start with all MPEG/DVB reserved PID's.
    for (PID pid = 0; pid <= PID_DVB_LAST; ++pid) {
        _demux.addPID(pid);
    }
}


//----------------------------------------------------------------------------
// Constructor for the PID context
//----------------------------------------------------------------------------

ts::TSAnalyzer::PIDContext::PIDContext(PID pid_, const UString& description_) :
    pid(pid_),
    description(description_),
    comment(),
    attributes(),
    services(),
    is_pmt_pid(false),
    is_pcr_pid(false),
    referenced(false),
    optional(false),
    carry_pes(false),
    carry_section(false),
    carry_ecm(false),
    carry_emm(false),
    carry_audio(false),
    carry_video(false),
    carry_t2mi(false),
    scrambled(false),
    same_stream_id(false),
    pes_stream_id(0),
    ts_pkt_cnt(0),
    ts_af_cnt(0),
    unit_start_cnt(0),
    pl_start_cnt(0),
    pmt_cnt(0),
    crypto_period(0),
    unexp_discont(0),
    exp_discont(0),
    duplicated(0),
    ts_sc_cnt(0),
    inv_ts_sc_cnt(0),
    inv_pes_start(0),
    t2mi_cnt(0),
    pcr_cnt(0),
    ts_pcr_bitrate(0),
    bitrate(0),
    language(),
    cas_id(0),
    cas_operators(),
    sections(),
    ssu_oui(),
    t2mi_plp_ts(),
    cur_continuity(0),
    cur_ts_sc(0),
    cur_ts_sc_pkt(0),
    cryptop_cnt(0),
    cryptop_ts_cnt(0),
    last_pcr(0),
    last_pcr_pkt(0),
    ts_bitrate_sum(0),
    ts_bitrate_cnt(0)
{
    // Guess the initial description, based on the PID
    // Global PID's (PAT, CAT, etc) are marked as "referenced" since they
    // should never be considered as orphan PID's. Optional PID's are known
    // PID's which should not appear in the report if no packet are found.

    switch (pid) {
        case PID_NULL:
            description = u"Stuffing";
            referenced = true;
            optional = true;
            break;
        case PID_PAT:
            description = u"PAT";
            referenced = true;
            carry_section = true;
            break;
        case PID_CAT:
            description = u"CAT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_TSDT:
            description = u"TSDT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_NIT:
            description = u"DVB-NIT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_SDT:
            description = u"SDT/BAT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_EIT:
            description = u"EIT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_RST:
            description = u"RST";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_TDT:
            description = u"TDT/TOT";
            referenced = true;
            optional = true;
            carry_section = true;
            break;
        case PID_NETSYNC:
            description = u"Network Synchronization";
            referenced = true;
            optional = true;
            break;
        case PID_RNT:
            description = u"RNT (TV-Anytime)";
            referenced = true;
            optional = true;
            break;
        case PID_INBSIGN:
            description = u"Inband Signalling";
            referenced = true;
            optional = true;
            break;
        case PID_MEASURE:
            description = u"Measurement";
            referenced = true;
            optional = true;
            break;
        case PID_DIT:
            description = u"DIT";
            referenced = true;
            optional = true;
            break;
        case PID_SIT:
            description = u"SIT";
            referenced = true;
            optional = true;
            break;
        default:
            break;
    }
}


//----------------------------------------------------------------------------
// Constructor for the ETID context
//----------------------------------------------------------------------------

ts::TSAnalyzer::ETIDContext::ETIDContext(const ETID& etid_) :
    etid(etid_),
    table_count(0),
    section_count(0),
    repetition_ts(0),
    min_repetition_ts(0),
    max_repetition_ts(0),
    first_version(0),
    last_version(0),
    versions(),
    first_pkt(0),
    last_pkt(0)
{
}


//----------------------------------------------------------------------------
// Constructor for the Service context
//----------------------------------------------------------------------------

ts::TSAnalyzer::ServiceContext::ServiceContext(uint16_t serv_id) :
    service_id(serv_id),
    orig_netw_id(0),
    service_type(0),
    name(),
    provider(),
    pmt_pid(0),
    pcr_pid(0),
    pid_cnt(0),
    scrambled_pid_cnt(0),
    ts_pkt_cnt(0),
    bitrate(0),
    carry_ssu(false),
    carry_t2mi(false)
{
}


//----------------------------------------------------------------------------
// Destructor for the Service context
//----------------------------------------------------------------------------

ts::TSAnalyzer::ServiceContext::~ServiceContext()
{
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
// Return an ETID context. Allocate a new entry if ETID not found.
//----------------------------------------------------------------------------

ts::TSAnalyzer::ETIDContextPtr ts::TSAnalyzer::getETID(const Section& section)
{
    const ETID etid = section.etid();
    const PIDContextPtr pc(getPID(section.sourcePID()));
    ETIDContextMap::const_iterator it(pc->sections.find(etid));

    if (it != pc->sections.end()) {
        // ETID context found
        return it->second;
    }
    else {
        ETIDContextPtr result(new ETIDContext(etid));
        pc->sections[etid] = result;
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
    if (p.isNull()) {
        // The PID was not yet used, map entry just created.
        return _pids[pid] = new PIDContext(pid, description);
    }
    else {
        return p;
    }
}


//----------------------------------------------------------------------------
//  Return a service context. Allocate a new entry if service not found.
//----------------------------------------------------------------------------

ts::TSAnalyzer::ServiceContextPtr ts::TSAnalyzer::getService(uint16_t service_id)
{
    ServiceContextPtr p(_services[service_id]);
    if (p.isNull()) {
        // The service was not yet used, map entry just created.
        return _services[service_id] = new ServiceContext(service_id);
    }
    else {
        return p;
    }
}


//----------------------------------------------------------------------------
//  Register a service into a PID description. The PID may belong to several
//  services, we add the service into this list, if not already in.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::PIDContext::addService(uint16_t service_id)
{
    // The PID now belongs to a service
    referenced = true;

    // Search the service in the list
    if (services.find(service_id) == services.end()) {
        // Service id not found, add it
        services.insert(service_id);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Implementation of SectionHandlerInterface
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleSection(SectionDemux&, const Section& section)
{
    ETIDContextPtr etc(getETID(section));
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
            PAT pat(table);
            if (pid == PID_PAT && pat.isValid()) {
                analyzePAT(pat);
            }
            break;
        }
        case TID_CAT: {
            CAT cat(table);
            if (pid == PID_CAT && cat.isValid()) {
                analyzeCAT(cat);
            }
            break;
        }
        case TID_PMT: {
            PMT pmt(table);
            if (pmt.isValid()) {
                analyzePMT(pid, pmt);
            }
            break;
        }
        case TID_SDT_ACT: {
            SDT sdt(table);
            if (sdt.isValid()) {
                analyzeSDT(sdt);
            }
            break;
        }
        case TID_TDT: {
            TDT tdt(table);
            if (tdt.isValid()) {
                analyzeTDT(tdt);
            }
            break;
        }
        case TID_TOT: {
            TOT tot(table);
            if (tot.isValid()) {
                analyzeTOT(tot);
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
    _ts_id_valid = true;

    // Get all PMT PID's for all services
    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
        uint16_t service_id(it->first);
        PID pmt_pid(it->second);
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
}


//----------------------------------------------------------------------------
// Analyze a CAT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeCAT(const CAT& cat)
{
    // Analyze the CA descritors to find EMM PIDs
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
    analyzeDescriptors(pmt.descs, svp.pointer());

    // Process all "elementary stream info"
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        const PID es_pid = it->first;
        const PMT::Stream& stream(it->second);
        ps = getPID(es_pid);
        ps->addService(pmt.service_id);
        ps->carry_audio = ps->carry_audio || IsAudioST(stream.stream_type);
        ps->carry_video = ps->carry_video || IsVideoST(stream.stream_type);
        ps->carry_pes = ps->carry_pes || IsPES(stream.stream_type);
        if (!ps->carry_section && !ps->carry_t2mi && IsSectionST(stream.stream_type)) {
            ps->carry_section = true;
            _demux.addPID(es_pid);
        }
        ps->description = names::StreamType(stream.stream_type);
        analyzeDescriptors(stream.descs, svp.pointer(), ps.pointer());
    }
}


//----------------------------------------------------------------------------
// Analyze an SDT
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeSDT(const SDT& sdt)
{
    // Register characteristics of all services
    for (SDT::ServiceMap::const_iterator it = sdt.services.begin(); it != sdt.services.end(); ++it) {

        ServiceContextPtr svp(getService(it->first)); // it->first = map key = service id
        svp->orig_netw_id = sdt.onetw_id;
        svp->service_type = it->second.serviceType();

        // Replace names only if they are not empty.
        const UString provider(it->second.providerName(_default_charset));
        const UString name(it->second.serviceName(_default_charset));
        if (!provider.empty()) {
            svp->provider = provider;
        }
        if (!name.empty()) {
            svp->name = name;
        }
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
// Return a full description, with comment and optionally attributes
//----------------------------------------------------------------------------

ts::UString ts::TSAnalyzer::PIDContext::fullDescription(bool include_attributes) const
{
    // Additional description
    UString more(comment);
    if (include_attributes) {
        for (UStringVector::const_iterator it = attributes.begin(); it != attributes.end(); ++it) {
            if (!it->empty()) {
                if (!more.empty()) {
                    more.append(u", ");
                }
                more.append(*it);
            }
        }
    }

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
//  Analyse a list of descriptors.
//  If svp is not 0, we are in the PMT of the specified service.
//  If ps is not 0, we are in the description of this PID in a PMT.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::analyzeDescriptors(const DescriptorList& descs, ServiceContext* svp, PIDContext* ps)
{
    for (size_t di = 0; di < descs.count(); ++di) {

        const uint8_t* data = descs[di]->payload();
        size_t size = descs[di]->payloadSize();

        switch (descs[di]->tag()) {
            case DID_CA: {
                analyzeCADescriptor(*descs[di], svp, ps);
                break;
            }
            case DID_LANGUAGE: {
                if (size >= 4 && ps != nullptr) {
                    // First 3 bytes contains the audio language
                    ps->language = UString::FromDVB(data, 3);
                    // Next byte contains audio type, 0 is the default
                    uint8_t audio_type(data[3]);
                    if (audio_type == 0) {
                        ps->comment = ps->language;
                    }
                    else {
                        ps->comment = ps->language + u", " + names::AudioType(audio_type);
                    }
                }
                break;
            }
            case DID_AC3: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an AC-3 audio track.
                    ps->description = u"AC-3 Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_ENHANCED_AC3: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an Enhanced AC-3 audio track.
                    ps->description = u"E-AC-3 Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_AAC: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates an HE-AAC audio track.
                    ps->description = u"HE-AAC Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_DTS: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates a DTS audio track.
                    ps->description = u"DTS Audio";
                    ps->carry_audio = true;
                }
                break;
            }
            case DID_SUBTITLING: {
                if (size >= 4 && ps != nullptr) {
                    // First 3 bytes contains the language
                    ps->language = UString::FromDVB(data, 3);
                    // Next byte contains subtitling type
                    uint8_t type = data[3];
                    ps->description = u"Subtitles";
                    ps->comment = ps->language;
                    AppendUnique(ps->attributes, names::SubtitlingType(type));
                }
                break;
            }
            case DID_TELETEXT: {
                if (size >= 4 && ps != nullptr) {
                    // First 3 bytes contains the language
                    ps->language = UString::FromDVB(data, 3);
                    // Next byte contains teletext type
                    uint8_t type(data[3] >> 3);
                    ps->description = u"Teletext";
                    ps->comment = ps->language;
                    AppendUnique(ps->attributes, names::TeletextType(type));
                }
                break;
            }
            case DID_APPLI_SIGNALLING: {
                if (ps != nullptr) {
                    // The presence of this descriptor indicates a PID carrying an AIT.
                    ps->comment = u"AIT";
                }
                break;
            }
            case DID_DVB_EXTENSION: {
                // Extension descriptor: need to look at the descriptor_tag_extension.
                if (size >= 1) {
                    switch (data[0]) {
                        case EDID_AC4: {
                            // The presence of this descriptor indicates an AC-4 audio track.
                            ps->description = u"AC-4 Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        case EDID_DTS_HD_AUDIO: {
                            // The presence of this descriptor indicates an DTS-HD audio track.
                            ps->description = u"DTS-HD Audio";
                            ps->carry_audio = true;
                            break;
                        }
                        case EDID_DTS_NEURAL: {
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
            case DID_DATA_BROADCAST_ID: {
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
                                ps->comment =  names::DataBroadcastId(dbid);
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

void ts::TSAnalyzer::analyzeCADescriptor(const Descriptor& desc, ServiceContext* svp, PIDContext* ps)
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
            eps->description.format(u"MediaGuard ECM for OPI %d (0x%X)", {opi, opi});
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
        eps->description.format(u"MediaGuard EMM for OPI %d (0x%X), EMM types: 0x%X", {opi, opi, etypes});
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
            eps1->description = UString::Format(u"MediaGuard Group EMM for OPI %d (0x%X)", {opi, opi});
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
                eps->description += UString::Format(u" for PPID %d (0x%X)", {ppid, ppid});
            }
            else {
                eps->description += UString::Format(u", %d (0x%X)", {ppid, ppid});
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
                    eps->description += UString::Format(u" for SOID %d (0x%06X)", {soid, soid});
                }
                else {
                    eps->description += UString::Format(u", %d (0x%06X)", {soid, soid});
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
            eps->description = names::CASId(ca_sysid) + u" EMM";
        }
        else {
            // Found an ECM PID for the service
            eps->carry_ecm = true;
            eps->addService(svp->service_id);
            eps->description = names::CASId(ca_sysid) + u" ECM";
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new audio attributes are found in an audio PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewAudioAttributes(PESDemux&, const PESPacket& pkt, const AudioAttributes& attr)
{
    AppendUnique(getPID(pkt.getSourcePID())->attributes, attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new AC-3 attributes are found in an audio PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewAC3Attributes(PESDemux&, const PESPacket& pkt, const AC3Attributes& attr)
{
    AppendUnique(getPID(pkt.getSourcePID())->attributes, attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new video attributes are found in a video PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewVideoAttributes(PESDemux&, const PESPacket& pkt, const VideoAttributes& attr)
{
    AppendUnique(getPID(pkt.getSourcePID())->attributes, attr.toString());
}


//----------------------------------------------------------------------------
// This hook is invoked when new AVC attributes are found in a video PID
// (Implementation of PESHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleNewAVCAttributes(PESDemux&, const PESPacket& pkt, const AVCAttributes& attr)
{
    AppendUnique(getPID(pkt.getSourcePID())->attributes, attr.toString());
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
    PIDContextPtr pc(getPID(pkt.getSourcePID(), u"T2-MI"));

    // Count T2-MI packets.
    pc->t2mi_cnt++;

    // Process PLP (only in baseband frame).
    if (pkt.plpValid()) {
        // Make sure the PLP is referenced, even if no TS packet is demux'ed.
        pc->t2mi_plp_ts[pkt.plp()];

        // Add the PLP as attributes of this PID.
        AppendUnique(pc->attributes, UString::Format(u"PLP: 0x%X (%d)", {pkt.plp(), pkt.plp()}));
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a new TS packet is extracted from T2-MI.
// (Implementation of T2MIHandlerInterface).
//----------------------------------------------------------------------------

void ts::TSAnalyzer::handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts)
{
    PIDContextPtr pc(getPID(t2mi.getSourcePID(), u"T2-MI"));

    // Count demux'ed TS packets from this PLP.
    pc->t2mi_plp_ts[t2mi.plp()]++;
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::feedPacket(const TSPacket& pkt)
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
                // Counter not following previous -> discountinuity
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

    // Process PCR
    if (broken_rate) {
        // Suspected packet loss, forget last PCR.
        ps->last_pcr = 0;
    }
    if (pkt.hasPCR()) {
        uint64_t pcr(pkt.getPCR());
        // Count PID's with PCR
        if (ps->pcr_cnt++ == 0)
            _pcr_pid_cnt++;
        // If last PCR valid, compute transport rate between the two
        if (ps->last_pcr != 0 && ps->last_pcr < pcr) {
            // Compute transport rate in b/s since last PCR
            uint64_t ts_bitrate =
                (uint64_t(packet_index - ps->last_pcr_pkt) * SYSTEM_CLOCK_FREQ * PKT_SIZE * 8) /
                (pcr - ps->last_pcr);
            // Per-PID statistics:
            ps->ts_bitrate_sum += ts_bitrate;
            ps->ts_bitrate_cnt++;
            // Transport stream statistics:
            _ts_bitrate_sum += ts_bitrate;
            _ts_bitrate_cnt++;
        }
        // Save PCR for next calculation
        ps->last_pcr = pcr;
        ps->last_pcr_pkt = packet_index;
    }

    // Check PES start code: PES packet headers start with the constant
    // sequence 00 00 01. Check this on all clear packets. This test is
    // actually meaningful only on TS packets carrying PES packets.
    // Note that "carrying PES" is an information that is not
    // available from the packet itself but from the environment
    // (for instance if the PID is referenced as a video PID in a PMT).
    // So, before getting the PMT referencing a PID, we do not know if
    // this PID carries PES or not.

    size_t header_size(pkt.getHeaderSize());

    if (pkt.getPUSI() && pkt.getScrambling() == SC_CLEAR && header_size <= PKT_SIZE - 3) {

        // Got a "unit start indicator" in a clear packet.
        // This may be the start of a section or a PES packet.

        if (pkt.b [header_size] != 0x00 || pkt.b [header_size + 1] != 0x00 || pkt.b [header_size + 2] != 0x01) {
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
                ps->pes_stream_id = pkt.b [header_size + 3];
                ps->same_stream_id = true;
            }
            else if (ps->pes_stream_id != pkt.b [header_size + 3]) {
                // Got different values of stream_id in PES packets
                ps->same_stream_id = false;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Specify a "bitrate hint" for the analysis. It is the user-specified
// bitrate in bits/seconds, based on 188-byte packets. The bitrate is
// optional: if specified as zero, the analysis is based on the PCR values.
//----------------------------------------------------------------------------

void ts::TSAnalyzer::setBitrateHint(BitRate bitrate)
{
    _ts_user_bitrate = bitrate;
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

    // Store "last" system times
    _last_utc = Time::CurrentUTC();
    _last_local = Time::CurrentLocalTime();

    // Compute bitrate and broadcast duration
    _ts_pcr_bitrate_188 = _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_sum / _ts_bitrate_cnt);
    _ts_pcr_bitrate_204 = _ts_bitrate_cnt == 0 ? 0 : BitRate((_ts_bitrate_sum * PKT_RS_SIZE) / (_ts_bitrate_cnt * PKT_SIZE));
    _ts_bitrate = _ts_user_bitrate != 0 ? _ts_user_bitrate : _ts_pcr_bitrate_188;
    _duration = _ts_bitrate == 0 ? 0 : (8000 * PKT_SIZE * uint64_t(_ts_pkt_cnt)) / _ts_bitrate;

    // Reinitialize all service information that will be updated PID by PID
    for (ServiceContextMap::iterator it = _services.begin(); it != _services.end(); ++it) {
        it->second->pid_cnt = 0;
        it->second->ts_pkt_cnt = 0;
        it->second->scrambled_pid_cnt = 0;
    }

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

    for (PIDContextMap::iterator pci = _pids.begin(); pci != _pids.end(); ++pci) {
        PIDContext& pc(*pci->second);

        // Compute TS bitrate from the PCR's of this PID
        if (pc.ts_bitrate_cnt != 0) {
            pc.ts_pcr_bitrate = uint32_t(pc.ts_bitrate_sum / pc.ts_bitrate_cnt);
        }

        // Compute average PID bitrate
        if (_ts_pkt_cnt != 0) {
            pc.bitrate = uint32_t((uint64_t(_ts_bitrate) * uint64_t(pc.ts_pkt_cnt)) / uint64_t(_ts_pkt_cnt));
        }

        // Compute average crypto-period for this PID
        // Remember that first crypto-period was ignored.
        if (pc.cryptop_cnt > 1) {
            pc.crypto_period = pc.cryptop_ts_cnt / (pc.cryptop_cnt - 1);
        }

        // If the PID belongs to some services, update services info.
        for (ServiceIdSet::iterator it = pc.services.begin(); it != pc.services.end(); ++it) {
            ServiceContextPtr scp(getService(*it));
            scp->pid_cnt++;
            scp->ts_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                scp->scrambled_pid_cnt++;
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
        }

        // Count global PID's
        if (pc.referenced && pc.services.size() == 0 && pc.ts_pkt_cnt != 0) {
            _global_pid_cnt++;
            _global_pkt_cnt += pc.ts_pkt_cnt;
            if (pc.scrambled) {
                _global_scr_pids++;
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
        _global_bitrate = uint32_t((uint64_t(_ts_bitrate) * uint64_t(_global_pkt_cnt)) / uint64_t(_ts_pkt_cnt));
        _psisi_bitrate = uint32_t((uint64_t(_ts_bitrate) * uint64_t(_psisi_pkt_cnt)) / uint64_t(_ts_pkt_cnt));
        _unref_bitrate = uint32_t((uint64_t(_ts_bitrate) * uint64_t(_unref_pkt_cnt)) / uint64_t(_ts_pkt_cnt));
    }

    // Complete all service information
    _scrambled_services_cnt = 0;

    for (ServiceContextMap::iterator sci = _services.begin(); sci != _services.end(); ++sci) {

        // Count scrambled services
        if (sci->second->scrambled_pid_cnt > 0) {
            _scrambled_services_cnt++;
        }

        // Compute average service bitrate
        if (_ts_pkt_cnt == 0) {
            sci->second->bitrate = 0;
        }
        else {
            sci->second->bitrate = uint32_t((uint64_t(_ts_bitrate) * uint64_t(sci->second->ts_pkt_cnt)) / uint64_t(_ts_pkt_cnt));
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

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {
        list.push_back(it->first);
    }
}


//----------------------------------------------------------------------------
// Return the list of PIDs
//----------------------------------------------------------------------------

void ts::TSAnalyzer::getPIDs(std::vector<PID>& list)
{
    recomputeStatistics();
    list.clear();

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        if (it->second->ts_pkt_cnt > 0) {
            list.push_back(it->first);
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

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        if (it->second->referenced && it->second->services.empty() && it->second->ts_pkt_cnt > 0) {
            list.push_back(it->first);
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

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        if (!it->second->referenced && it->second->ts_pkt_cnt > 0) {
            list.push_back(it->first);
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

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        if (it->second->services.count(service_id) > 0) {
            list.push_back(it->first);
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

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        if (it->second->carry_pes) {
            list.push_back(it->first);
        }
    }
}

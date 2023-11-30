//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSignalizationDemux.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsPESPacket.h"
#include "tsLogicalChannelNumbers.h"
#include "tsCADescriptor.h"
#include "tsISDBAccessControlDescriptor.h"
#include "tsCAT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsRST.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsTSDT.h"
#include "tsMGT.h"
#include "tsCVCT.h"
#include "tsTVCT.h"
#include "tsRRT.h"
#include "tsSTT.h"
#include "tsSAT.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SignalizationDemux::SignalizationDemux(DuckContext& duck) :
    SignalizationDemux(duck, nullptr)
{
    _full_filters = true;
    addFullFilters();
}

ts::SignalizationDemux::SignalizationDemux(DuckContext& duck, SignalizationHandlerInterface* handler, std::initializer_list<TID> tids) :
    _duck(duck),
    _demux(duck, this, this),
    _handler(handler)
{
    _last_pat.invalidate();
    for (const auto& it : tids) {
        addFilteredTableId(it);
    }
}


//----------------------------------------------------------------------------
// Reset the demux.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::reset()
{
    _demux.reset();
    _demux.setPIDFilter(NoPID);
    _filtered_tids.clear();
    _filtered_srv_ids.clear();
    _filtered_srv_names.clear();
    _last_pat.invalidate();
    _last_pat_handled = false;
    _last_nit.invalidate();
    _last_nit_handled = false;
    _ts_id = _orig_network_id = _network_id = 0xFFFF;
    _last_utc.clear();
    _pids.clear();
    _services.clear();

    // Apply full filters when set by default.
    if (_full_filters) {
        addFullFilters();
    }
}


//----------------------------------------------------------------------------
// This method feeds the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::feedPacket(const TSPacket& pkt)
{
    // Keep statistics on the PID.
    auto ctx(getPIDContext(pkt.getPID()));
    if (pkt.getPUSI()) {
        // The packet contains a payload unit start.
        if (ctx->first_pusi == INVALID_PACKET_COUNTER) {
            ctx->first_pusi = ctx->packets;
        }
        ctx->last_pusi = ctx->packets;
        ctx->pusi_count++;
        if (pkt.hasPayload() && PESPacket::FindIntraImage(pkt.getPayload(), pkt.getPayloadSize(), ctx->stream_type, ctx->codec) != NPOS) {
            // The payload contains the start of an intra image.
            if (ctx->first_intra == INVALID_PACKET_COUNTER) {
                ctx->first_intra = ctx->packets;
            }
            ctx->last_intra = ctx->packets;
            ctx->intra_count++;
        }
    }
    ctx->packets++;
    if (pkt.getScrambling() != SC_CLEAR) {
        ctx->scrambled = true;
    }

    // Feed to table demux to collect signalization.
    _demux.feedPacket(pkt);
}


//----------------------------------------------------------------------------
// Get the characteristics of the PID's.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::getPIDs(PIDSet& pids) const
{
    pids.reset();
    for (const auto& it : _pids) {
        if (it.second->packets > 0) {
            pids.set(it.first);
        }
    }
}

ts::PID ts::SignalizationDemux::nitPID() const
{
    // Get the NIT PID, either from last PAT or default PID.
    return _last_pat.isValid() && _last_pat.nit_pid != PID_NULL ? _last_pat.nit_pid : PID(PID_NIT);
}

ts::PIDClass ts::SignalizationDemux::pidClass(PID pid, PIDClass defclass) const
{
    auto ctx = _pids.find(pid);
    const PIDClass pclass = ctx == _pids.end() ? PIDClass::UNDEFINED : ctx->second->pid_class;
    return pclass == PIDClass::UNDEFINED ? defclass : pclass;
}

ts::CodecType ts::SignalizationDemux::codecType(PID pid, CodecType deftype) const
{
    auto ctx = _pids.find(pid);
    const CodecType type = ctx == _pids.end() ? CodecType::UNDEFINED : ctx->second->codec;
    return type == CodecType::UNDEFINED ? deftype : type;
}

uint8_t ts::SignalizationDemux::streamType(PID pid, uint8_t deftype) const
{
    auto ctx = _pids.find(pid);
    const uint8_t type = ctx == _pids.end() ? uint8_t(ST_NULL) : ctx->second->stream_type;
    return type == ST_NULL ? deftype : type;
}

bool ts::SignalizationDemux::isScrambled(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx != _pids.end() && ctx->second->scrambled;
}

ts::PacketCounter ts::SignalizationDemux::packetCount(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? 0 : ctx->second->packets;
}

ts::PacketCounter ts::SignalizationDemux::pusiCount(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? 0 : ctx->second->pusi_count;
}

ts::PacketCounter ts::SignalizationDemux::pusiFirstIndex(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? INVALID_PACKET_COUNTER : ctx->second->first_pusi;
}

ts::PacketCounter ts::SignalizationDemux::pusiLastIndex(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? INVALID_PACKET_COUNTER : ctx->second->last_pusi;
}

ts::PacketCounter ts::SignalizationDemux::intraFrameCount(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? 0 : ctx->second->intra_count;
}

ts::PacketCounter ts::SignalizationDemux::intraFrameFirstIndex(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? INVALID_PACKET_COUNTER : ctx->second->first_intra;
}

ts::PacketCounter ts::SignalizationDemux::intraFrameLastIndex(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx == _pids.end() ? INVALID_PACKET_COUNTER : ctx->second->last_intra;
}

bool ts::SignalizationDemux::atIntraFrame(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx != _pids.end() && ctx->second->intra_count > 0 && ctx->second->packets - 1 == ctx->second->last_intra;
}

bool ts::SignalizationDemux::inService(PID pid, uint16_t service_id) const
{
    auto ctx = _pids.find(pid);
    return ctx != _pids.end() && Contains(ctx->second->services, service_id);
}

bool ts::SignalizationDemux::inAnyService(PID pid, std::set<uint16_t> service_ids) const
{
    auto ctx = _pids.find(pid);
    if (ctx != _pids.end()) {
        for (auto it : service_ids) {
            if (Contains(ctx->second->services, it)) {
                return true;
            }
        }
    }
    return false;
}

uint16_t ts::SignalizationDemux::serviceId(PID pid) const
{
    auto ctx = _pids.find(pid);
    return ctx != _pids.end() && !ctx->second->services.empty() ? *ctx->second->services.begin() : 0xFFFF;
}

void ts::SignalizationDemux::getServiceIds(PID pid, std::set<uint16_t> services) const
{
    auto ctx = _pids.find(pid);
    if (ctx == _pids.end()) {
        services.clear();
    }
    else {
        services = ctx->second->services;
    }
}


//----------------------------------------------------------------------------
// Get the characteristics of the services.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::getServiceIds(std::set<uint16_t>& services) const
{
    services.clear();
    for (const auto& it : _services) {
        services.insert(it.first);
    }
}

void ts::SignalizationDemux::getServices(ServiceList& services) const
{
    services.clear();
    for (const auto& it : _services) {
        services.push_back(it.second->service);
    }
}


//----------------------------------------------------------------------------
// Add table filtering for full services and PID's analysis.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::addFullFilters()
{
    addFilteredTableIds({TID_PAT, TID_CAT, TID_PMT, TID_NIT_ACT, TID_SDT_ACT, TID_TDT, TID_TOT, TID_MGT, TID_CVCT, TID_TVCT, TID_STT});
}


//----------------------------------------------------------------------------
// Add a signalization table id to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::addFilteredTableIds(std::initializer_list<TID> tids)
{
    for (auto it : tids) {
        addFilteredTableId(it);
    }
}

bool ts::SignalizationDemux::addFilteredTableId(TID tid)
{
    // Do not repeat already filtered table ids.
    if (isFilteredTableId(tid)) {
        return true;
    }

    // Configure the demux according to the table id.
    switch (tid) {
        case TID_PAT: {
            _demux.addPID(PID_PAT);
            // The current PAT may have already been received without notification to the application.
            if (_last_pat.isValid() && _handler != nullptr && !_last_pat_handled) {
                _last_pat_handled = true;
                _handler->handlePAT(_last_pat, PID_PAT);
            }
            break;
        }
        case TID_CAT: {
            _demux.addPID(PID_CAT);
            break;
        }
        case TID_PMT: {
            // We need the PAT to get PMT PID's.
            _demux.addPID(PID_PAT);
            // If a PAT is known, add all PMT PID's.
            if (_last_pat.isValid()) {
                for (const auto& it : _last_pat.pmts) {
                    _demux.addPID(it.second);
                }
            }
            break;
        }
        case TID_TSDT: {
            _demux.addPID(PID_TSDT);
            break;
        }
        case TID_NIT_ACT:
        case TID_NIT_OTH:  {
            // We need the PAT to get the NIT PID.
            _demux.addPID(PID_PAT);
            _demux.addPID(nitPID());
            break;
        }
        case TID_SDT_ACT:
        case TID_SDT_OTH:
        case TID_BAT: {
            // SDT and BAT share the same PID.
            _demux.addPID(PID_SDT);
            break;
        }
        case TID_RST: {
            _demux.addPID(PID_RST);
            break;
        }
        case TID_TDT:
        case TID_TOT: {
            // TDT and TOT share the same PID.
            _demux.addPID(PID_TDT);
            break;
        }
        case TID_MGT:
        case TID_CVCT:
        case TID_TVCT:
        case TID_RRT:
        case TID_STT: {
            // With ATSC, the PSIP base PID contains almost all tables.
            _demux.addPID(PID_PSIP);
            break;
        }
        case TID_SAT: {
            _demux.addPID(PID_SAT);
            break;
        }
        default: {
            // Unsupported table id.
            return false;
        }
    }

    // Add the table id.
    _filtered_tids.insert(tid);
    return true;
}


//----------------------------------------------------------------------------
// Remove a signalization table id to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeFilteredTableIds(std::initializer_list<TID> tids)
{
    for (auto it : tids) {
        removeFilteredTableId(it);
    }
}

bool ts::SignalizationDemux::removeFilteredTableId(TID tid)
{
    // Do nothing if the table id was not filtered.
    if (!isFilteredTableId(tid)) {
        return false;
    }

    // Remove the table id first.
    _filtered_tids.erase(tid);

    // Configure the demux according to the table id.
    switch (tid) {
        case TID_PAT: {
            // Stop monitoring the PAT only when there is no need to get PMT's or NIT.
            if (!isFilteredTableId(TID_PMT) &&
                _filtered_srv_ids.empty() &&
                _filtered_srv_names.empty() &&
                !isFilteredTableId(TID_NIT_ACT) &&
                !isFilteredTableId(TID_NIT_OTH))
            {
                _demux.removePID(PID_PAT);
            }
            break;
        }
        case TID_CAT: {
            _demux.removePID(PID_CAT);
            break;
        }
        case TID_PMT: {
            // If a PAT is known, remove all PMT PID's which are not specifically monitored by service id.
            if (_last_pat.isValid()) {
                for (const auto& it : _last_pat.pmts) {
                    if (!isFilteredServiceId(it.first)) {
                        _demux.removePID(it.second);
                    }
                }
            }
            break;
        }
        case TID_TSDT: {
            _demux.removePID(PID_TSDT);
            break;
        }
        case TID_NIT_ACT:
        case TID_NIT_OTH: {
            // Remove the PID only if no type of NIT is monitored.
            if (!isFilteredTableId(TID_NIT_ACT) && !isFilteredTableId(TID_NIT_OTH)) {
                _demux.removePID(nitPID());
            }
            break;
        }
        case TID_SDT_ACT:
        case TID_SDT_OTH:
        case TID_BAT: {
            // SDT and BAT share the same PID. Remove the PID only if none is monitored.
            if (!isFilteredTableId(TID_SDT_ACT) && !isFilteredTableId(TID_SDT_OTH) && !isFilteredTableId(TID_BAT)) {
                _demux.removePID(PID_SDT);
            }
            break;
        }
        case TID_RST: {
            _demux.removePID(PID_RST);
            break;
        }
        case TID_TDT:
        case TID_TOT: {
            // TDT and TOT share the same PID. Remove the PID only if none is monitored.
            if (!isFilteredTableId(TID_TDT) && !isFilteredTableId(TID_TOT)) {
                _demux.removePID(PID_TDT);
            }
            break;
        }
        case TID_MGT:
        case TID_CVCT:
        case TID_TVCT:
        case TID_RRT:
        case TID_STT: {
            // With ATSC, the PSIP base PID contains almost all tables.
            if (!isFilteredTableId(TID_MGT) && !isFilteredTableId(TID_CVCT) && !isFilteredTableId(TID_TVCT) && !isFilteredTableId(TID_RRT) && !isFilteredTableId(TID_STT)) {
                _demux.removePID(PID_PSIP);
            }
            break;
        }
        case TID_SAT: {
            _demux.removePID(PID_SAT);
            break;
        }
        default: {
            // Unsupported table id.
            return false;
        }
    }

    // Table id successfully removed.
    return true;
}


//----------------------------------------------------------------------------
// Add a service id to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::addFilteredServiceId(uint16_t sid)
{
    // Do something only when the service is not yet monitored by service.
    if (!isFilteredServiceId(sid)) {

        // Remember the service id to monitor.
        _filtered_srv_ids.insert(sid);

        // We need the PAT to get PMT PID's.
        _demux.addPID(PID_PAT);

        // If a PAT is known and references the service, add its PMT PID.
        if (_last_pat.isValid()) {
            const auto it(_last_pat.pmts.find(sid));
            if (it != _last_pat.pmts.end()) {
                _demux.addPID(it->second);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Remove a service id to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeFilteredServiceId(uint16_t sid)
{
    // Do something only when the service is currently monitored.
    if (isFilteredServiceId(sid)) {

        // Forget the service id to monitor.
        _filtered_srv_ids.erase(sid);

        // If a PAT is known and references the service, remove its PMT PID.
        // If all PMT's are still monitored, don't change anything.
        if (_last_pat.isValid() && !isFilteredTableId(TID_PMT)) {
            const auto it(_last_pat.pmts.find(sid));
            if (it != _last_pat.pmts.end()) {
                _demux.removePID(it->second);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Add a service to filter, by name or by id.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::addFilteredService(const UString& name)
{
    uint16_t id = 0;
    if (name.toInteger(id, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // This is a service id.
        addFilteredServiceId(id);
    }
    else {
        // Add a service by name. Check if already in the list of filtered names.
        for (const auto& it : _filtered_srv_names) {
            if (it.similar(name)) {
                return;
            }
        }
        // Add in the list of filtered names.
        _filtered_srv_names.insert(name);
        // Then, if the service id is known, filter its service ids.
        for (const auto& it : _services) {
            if (it.second->service.match(name)) {
                addFilteredServiceId(it.first);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Remove a service to filter, by name or by id.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeFilteredService(const UString& name)
{
    uint16_t id = 0;
    if (name.toInteger(id, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // This is a service id.
        removeFilteredServiceId(id);
    }
    else {
        // Remove a service by name. First remove it from the list of filtered names.
        for (auto it = _filtered_srv_names.begin(); it != _filtered_srv_names.end(); ) {
            if (!it->similar(name)) {
                ++it;
            }
            else {
                it = _filtered_srv_names.erase(it);
            }
        }
        // Then, if the service id is known, remove its service ids.
        for (const auto& it : _services) {
            if (it.second->service.match(name)) {
                removeFilteredServiceId(it.first);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Check if a service name is filtered.
//----------------------------------------------------------------------------

bool ts::SignalizationDemux::isFilteredServiceName(const UString& name) const
{
    uint16_t id = 0;
    if (name.toInteger(id, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // This is a service id.
        return isFilteredServiceId(id);
    }
    else {
        // Find similar names.
        for (const auto& it : _filtered_srv_names) {
            if (it.similar(name)) {
                return true;
            }
        }
        return false;
    }
}


//----------------------------------------------------------------------------
// Remove all service to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeAllFilteredServices()
{
    // If a PAT is known, remove all PMT PID's.
    // If all PMT's are still monitored, don't change anything.
    if (_last_pat.isValid() && !isFilteredTableId(TID_PMT)) {
        for (const auto& it : _last_pat.pmts) {
            _demux.removePID(it.second);
        }
    }

    // Forget all service ids.
    _filtered_srv_ids.clear();
    _filtered_srv_names.clear();
}


//----------------------------------------------------------------------------
// Invoked by SectionDemux when a complete table is received.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleTable(SectionDemux&, const BinaryTable& table)
{
    _duck.report().debug(u"signalization demux got table id 0x%X (%<d)", {table.tableId()});

    const PID pid = table.sourcePID();
    const TID tid = table.tableId();

    switch (tid) {
        case TID_PAT: {
            const PAT pat(_duck, table);
            if (pat.isValid() && pid == PID_PAT) {
                handlePAT(pat, pid);
            }
            break;
        }
        case TID_CAT: {
            const CAT cat(_duck, table);
            if (cat.isValid() && pid == PID_CAT) {
                handleCAT(cat, pid);
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                handlePMT(pmt, pid);
            }
            break;
        }
        case TID_TSDT: {
            const TSDT tsdt(_duck, table);
            if (tsdt.isValid() && pid == PID_TSDT && _handler != nullptr && isFilteredTableId(TID_TSDT)) {
                _handler->handleTSDT(tsdt, pid);
            }
            break;
        }
        case TID_NIT_ACT:
        case TID_NIT_OTH:  {
            const NIT nit(_duck, table);
            if (nit.isValid() && pid == nitPID()) {
                handleNIT(nit, pid);
            }
            break;
        }
        case TID_SDT_ACT:
        case TID_SDT_OTH:  {
            const SDT sdt(_duck, table);
            if (sdt.isValid() && pid == PID_SDT) {
                handleSDT(sdt, pid);
            }
            break;
        }
        case TID_BAT: {
            const BAT bat(_duck, table);
            if (bat.isValid() && pid == PID_BAT && _handler != nullptr && isFilteredTableId(tid)) {
                _handler->handleBAT(bat, pid);
            }
            break;
        }
        case TID_RST: {
            const RST rst(_duck, table);
            if (rst.isValid() && pid == PID_RST && _handler != nullptr && isFilteredTableId(tid)) {
                _handler->handleRST(rst, pid);
            }
            break;
        }
        case TID_TDT: {
            const TDT tdt(_duck, table);
            if (tdt.isValid() && pid == PID_TDT) {
                _last_utc = tdt.utc_time;
                if (_handler != nullptr && isFilteredTableId(tid)) {
                    _handler->handleTDT(tdt, pid);
                }
                if (_handler != nullptr) {
                    _handler->handleUTC(_last_utc, tid);
                }
            }
            break;
        }
        case TID_TOT: {
            const TOT tot(_duck, table);
            if (tot.isValid() && pid == PID_TOT) {
                _last_utc = tot.utc_time;
                if (_handler != nullptr && isFilteredTableId(tid)) {
                    _handler->handleTOT(tot, pid);
                }
                if (_handler != nullptr) {
                    _handler->handleUTC(_last_utc, tid);
                }
            }
            break;
        }
        case TID_MGT: {
            const MGT mgt(_duck, table);
            if (mgt.isValid() && pid == PID_PSIP) {
                handleMGT(mgt, pid);
            }
            break;
        }
        case TID_CVCT: {
            const CVCT vct(_duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct, pid, &SignalizationHandlerInterface::handleCVCT);
            }
            break;
        }
        case TID_TVCT: {
            const TVCT vct(_duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct, pid, &SignalizationHandlerInterface::handleTVCT);
            }
            break;
        }
        case TID_RRT: {
            const RRT rrt(_duck, table);
            if (rrt.isValid() && pid == PID_PSIP && _handler != nullptr && isFilteredTableId(tid)) {
                _handler->handleRRT(rrt, pid);
            }
            break;
        }
        case TID_SAT: {
            const SAT sat(_duck, table);
            if (sat.isValid() && pid == PID_SAT) {
                handleSAT(sat, pid);
            }
            break;
        }
        default: {
            // Unsupported table id or processed elsewhere (STT).
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by SectionDemux when a section is received.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleSection(SectionDemux&, const Section& section)
{
    // We use this handler for ATSC System Time Table (STT) only.
    // This table violates the common usage rules of MPEG sections, see file tsSTT.h.
    if (section.tableId() == TID_STT && section.sourcePID() == PID_PSIP) {
        const STT stt(_duck, section);
        if (stt.isValid()) {
            _last_utc = stt.utcTime();
            if (_handler != nullptr && isFilteredTableId(TID_STT)) {
                _handler->handleSTT(stt, PID_PSIP);
            }
            if (_handler != nullptr) {
                _handler->handleUTC(_last_utc, TID_STT);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a PAT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handlePAT(const PAT& pat, PID pid)
{
    // Check if all PMT's are monitored.
    const bool all_pmts = isFilteredTableId(TID_PMT);

    // If a previous PAT was there, remove unused PMT PID's.
    if (_last_pat.isValid() && (all_pmts || !_filtered_srv_ids.empty())) {
        // Loop on all previous services
        for (const auto& old_it : _last_pat.pmts) {
            // If the service no longer exists or has changed its PMT PID, remove the previous PMT PID. from the demux.
            const auto new_it = pat.pmts.find(old_it.first);
            if (new_it == pat.pmts.end() || new_it->second != old_it.second) {
                _demux.removePID(old_it.second);
            }
        }
    }

    // Remember the last PAT.
    _last_pat = pat;
    _last_pat_handled = false;
    _ts_id = pat.ts_id;

    // Notify the PAT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_PAT)) {
        _last_pat_handled = true;
        _handler->handlePAT(pat, pid);
    }

    // Add or update services.
    for (const auto& pat_it : pat.pmts) {
        // Monitor new PMT PID's. Some of them may be already monitored.
        if (all_pmts || isFilteredServiceId(pat_it.first)) {
            _demux.addPID(pat_it.second);
        }
        // Update the service PMT PID.
        auto srv = getServiceContext(pat_it.first, CreateService::ALWAYS);
        srv->service.setPMTPID(pat_it.second);
        srv->service.setTSId(pat.ts_id);
        // Notify application if the service was just created or its PMT changed.
        if (_handler != nullptr && srv->service.isModified()) {
            _handler->handleService(_ts_id, srv->service, srv->pmt, false);
            srv->service.clearModified();
        }
    }

    // Monitor non-standard NIT PID.
    if (isFilteredTableId(TID_NIT_ACT) || isFilteredTableId(TID_NIT_OTH)) {
        _demux.addPID(nitPID());
    }

    // Remove all services which are no longer here.
    for (auto srv_it = _services.begin(); srv_it != _services.end(); ) {
        auto pat_it = pat.pmts.find(srv_it->first);
        if (pat_it == pat.pmts.end()) {
            // Service has an id which is not in the PAT => remove from the service list.
            if (_handler != nullptr) {
                _handler->handleService(_ts_id, srv_it->second->service, srv_it->second->pmt, true);
            }
            srv_it = _services.erase(srv_it);
        }
        else {
            // Keep the service, move to next one.
            ++srv_it;
        }
    }

    // Reprocess the last NIT in case of PAT change (TS id may have changed).
    if (_last_nit.isValid() && !_last_nit_handled) {
        handleNIT(_last_nit, nitPID());
    }
}


//----------------------------------------------------------------------------
// Process a CAT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleCAT(const CAT& cat, PID pid)
{
    // Notify the CAT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_CAT)) {
        _handler->handleCAT(cat, pid);
    }

    // Look for EMM PID's in the CAT.
    handleDescriptors(cat.descs, pid);
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handlePMT(const PMT& pmt, PID pid)
{
    // Ignore PMT for unknown service. If we expect a PMT, there was a filter
    // on its PID because we found that PID in the PAT. At that time, all
    // services from the PAT were registered in _services.
    auto srv(getServiceContext(pmt.service_id, CreateService::NEVER));
    if (srv.isNull()) {
        return;
    }

    // Register the PMT in the service.
    srv->pmt = pmt;
    srv->service.setPMTPID(pid);

    // In case of PMT update for an existing service, remove all previous PID's for this service.
    for (const auto& it : _pids) {
        it.second->services.erase(pmt.service_id);
    }

    // Register the PMT PID as PSI.
    auto ctx(getPIDContext(pid));
    ctx->pid_class = PIDClass::PSI;
    ctx->services.insert(pmt.service_id);

    // Notify the PMT to the application.
    if (_handler != nullptr && (isFilteredTableId(TID_PMT) || isFilteredServiceId(pmt.service_id))) {
        _handler->handlePMT(pmt, pid);
    }

    // Look for ECM PID's at service level.
    handleDescriptors(pmt.descs, pid);

    // Loop on all components.
    for (const auto& it : pmt.streams) {

        // Register the characteritics of the component PID.
        ctx = getPIDContext(it.first);
        ctx->pid_class = it.second.getClass(_duck);
        ctx->stream_type = it.second.stream_type;
        ctx->codec = it.second.getCodec(_duck);
        ctx->services.insert(pmt.service_id);

        // Look for ECM PID's at component level.
        handleDescriptors(pmt.descs, pid);
    }

    // A PMT change always means that something has changed in the service.
    if (_handler != nullptr) {
        _handler->handleService(_ts_id, srv->service, srv->pmt, false);
        srv->service.clearModified();
    }
}


//----------------------------------------------------------------------------
// Process a NIT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleNIT(const NIT& nit, PID pid)
{
    // Extract information on this TS only on the NIT Actual.
    if (nit.isActual()) {
        _network_id = nit.network_id;

        // Remember the last NIT (if not the reprocessing of the last NIT)
        if (&nit != &_last_nit) {
            _last_nit = nit;
        }
        _last_nit_handled = false;

        // If there is no PAT, we don't know the TS id, reprocess later.
        if (!_last_pat.isValid()) {
            return;
        }
    }

    // Notify the NIT to the application.
    if (_handler != nullptr && isFilteredTableId(nit.tableId())) {
        _last_nit_handled = _last_nit_handled || nit.isActual();
        _handler->handleNIT(nit, pid);
    }

    // Process modifications on services from the NIT.
    if (nit.isActual()) {
        // Collect all logical channel numbers from the NIT for the TS id.
        LogicalChannelNumbers lcn(_duck);
        lcn.addFromNIT(nit, _ts_id);

        // Update LCN's in our services.
        ServiceContextMapView services_view(_services, _ts_id, _orig_network_id);
        lcn.updateServices(services_view, Replacement::UPDATE);

        // Check which services were modified and notify application.
        if (_handler != nullptr) {
            for (auto& srv : _services) {
                if (srv.second->service.isModified()) {
                    _handler->handleService(_ts_id, srv.second->service, srv.second->pmt, false);
                    srv.second->service.clearModified();
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process an SDT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleSDT(const SDT& sdt, PID pid)
{
    // Notify the SDT to the application.
    if (_handler != nullptr && isFilteredTableId(sdt.tableId())) {
        _handler->handleSDT(sdt, pid);
    }

    // Extract information on this TS only on the SDT Actual.
    if (sdt.isActual()) {

        // Get transport stream identification.
        _ts_id = sdt.ts_id;
        _orig_network_id = sdt.onetw_id;

        // Collect service information. Loop on all services in the SDT.
        for (const auto& sdt_it : sdt.services) {
            // Find existing services (the PAT is known) or may exist (the PAT is not yet known).
            // When the PAT is received later and the service does not exist, it will be removed.
            const auto srv(getServiceContext(sdt_it.first, CreateService::IF_MAY_EXIST));
            if (!srv.isNull()) {
                sdt_it.second.updateService(_duck, srv->service);
                // If the service description changed, notify the application.
                if (_handler != nullptr && srv->service.isModified()) {
                    _handler->handleService(_ts_id, srv->service, srv->pmt, false);
                    srv->service.clearModified();
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process an MGT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleMGT(const MGT& mgt, PID pid)
{
    // Notify the MGT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_MGT)) {
        _handler->handleMGT(mgt, pid);
    }

    // Locate all additional ATSC signalization PID's.
    for (const auto& it : mgt.tables) {
        getPIDContext(it.second.table_type_PID)->pid_class = PIDClass::PSI;
    }
}


//----------------------------------------------------------------------------
// Process a VCT (TVCT or CVCT).
//----------------------------------------------------------------------------

template <class XVCT, typename std::enable_if<std::is_base_of<ts::VCT, XVCT>::value, int>::type>
void ts::SignalizationDemux::handleVCT(const XVCT& vct, PID pid, void (SignalizationHandlerInterface::*handle)(const XVCT&, PID))
{
    // Call specific and generic form of VCT handler.
    if (_handler != nullptr && isFilteredTableId(vct.tableId())) {
        (_handler->*handle)(vct, pid);
        _handler->handleVCT(vct, pid);
    }

    // Collect service information. Loop on all services in the VCT.
    for (const auto& vct_it : vct.channels) {
        // Find existing services (the PAT is known) or may exist (the PAT is not yet known).
        // When the PAT is received later and the service does not exist, it will be removed.
        const auto srv(getServiceContext(vct_it.second.program_number, CreateService::IF_MAY_EXIST));
        if (!srv.isNull()) {
            vct_it.second.updateService(srv->service);
            // If the service description changed, notify the application.
            if (_handler != nullptr && srv->service.isModified()) {
                _handler->handleService(_ts_id, srv->service, srv->pmt, false);
                srv->service.clearModified();
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a SAT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleSAT(const SAT& sat, PID pid)
{
    // Notify the SAT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_SAT)) {
        _handler->handleSAT(sat, pid);
    }
}


//----------------------------------------------------------------------------
// Process a descriptor list, looking for useful information.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleDescriptors(const DescriptorList& dlist, PID pid)
{
    // Loop on all descriptors
    for (size_t index = 0; index < dlist.size(); ++index) {
        const DescriptorPtr& ptr(dlist[index]);
        if (!ptr.isNull() && ptr->isValid()) {
            const DID did = ptr->tag();

            // Extract descriptor-dependent information.
            if (did == DID_CA) {
                const CADescriptor desc(_duck, *ptr);
                if (desc.isValid()) {
                    getPIDContext(desc.ca_pid)->setCAS(dlist.table(), desc.cas_id);
                }
            }
            else if (bool(_duck.standards() & Standards::ISDB) && did == DID_ISDB_CA) {
                const ISDBAccessControlDescriptor desc(_duck, *ptr);
                if (desc.isValid()) {
                    getPIDContext(desc.pid)->setCAS(dlist.table(), desc.CA_system_id);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// PID context
//----------------------------------------------------------------------------

// Get the context for a PID. Create if not existent.
ts::SignalizationDemux::PIDContextPtr ts::SignalizationDemux::getPIDContext(PID pid)
{
    auto it = _pids.find(pid);
    return it != _pids.end() ? it->second : (_pids[pid] = PIDContextPtr(new PIDContext(pid)));
}

// Constructor.
ts::SignalizationDemux::PIDContext::PIDContext(PID pid_) :
    pid(pid_)
{
    if (pid == PID_NULL) {
        pid_class = PIDClass::STUFFING;
    }
    else if (pid <= PID_ISDB_LAST || pid >= PID_ATSC_FIRST) {
        pid_class = PIDClass::PSI;
    }
}

// Register a CAS type from a table.
void ts::SignalizationDemux::PIDContext::setCAS(const AbstractTable* table, uint16_t cas)
{
    cas_id = cas;
    if (table != nullptr) {
        if (table->tableId() == TID_CAT) {
            pid_class = PIDClass::EMM;
        }
        else if (table->tableId() == TID_PMT) {
            pid_class = PIDClass::ECM;
            const PMT* pmt = dynamic_cast<const PMT*>(table);
            if (pmt != nullptr) {
                services.insert(pmt->service_id);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Service context
//----------------------------------------------------------------------------

// Get the context for a service. Create if not existent.
ts::SignalizationDemux::ServiceContextPtr ts::SignalizationDemux::getServiceContext(uint16_t service_id, CreateService create)
{
    auto it = _services.find(service_id);
    if (it != _services.end()) {
        return it->second;
    }
    else if (create == CreateService::ALWAYS ||
             (create == CreateService::IF_MAY_EXIST && !_last_pat.isValid()) ||
             (create == CreateService::IF_MAY_EXIST && _last_pat.isValid() && Contains(_last_pat.pmts, service_id)))
    {
        return _services[service_id] = ServiceContextPtr(new ServiceContext(service_id));
    }
    else {
        return ServiceContextPtr();
    }
}

// Constructor.
ts::SignalizationDemux::ServiceContext::ServiceContext(uint16_t service_id)
{
    service.setId(service_id); // must no be set in constructor, must set "modified"
    pmt.invalidate();
}

// A view of ServiceContextMap which iterates over Service fields.
// Add a service only if it comes from the same TS.
void ts::SignalizationDemux::ServiceContextMapView::push_back(const Service& srv)
{
    if (_tsid != 0xFFFF && (!srv.hasTSId() || srv.hasTSId(_tsid)) && (_onid == 0xFFFF || !srv.hasONId() || srv.hasONId(_onid))) {
        if (_svmap[srv.getId()].isNull()) {
            _svmap[srv.getId()] = ServiceContextPtr(new ServiceContext(srv.getId()));
        }
        _svmap[srv.getId()]->service = srv;
    }
}

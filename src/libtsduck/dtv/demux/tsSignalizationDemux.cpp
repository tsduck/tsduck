//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsSignalizationDemux.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsCADescriptor.h"
#include "tsISDBAccessControlDescriptor.h"
TSDUCK_SOURCE;


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
    _handler(handler),
    _full_filters(false),
    _tids(),
    _service_ids(),
    _last_pat(),
    _last_pat_handled(false),
    _last_nit(),
    _last_nit_handled(false),
    _ts_id(0xFFFF),
    _orig_network_id(0xFFFF),
    _network_id(0xFFFF),
    _last_utc(),
    _pids(),
    _services()
{
    _last_pat.invalidate();
    for (auto it = tids.begin(); it != tids.end(); ++it) {
        addFilteredTableId(*it);
    }
}


//----------------------------------------------------------------------------
// Reset the demux.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::reset()
{
    _demux.reset();
    _demux.setPIDFilter(NoPID);
    _tids.clear();
    _service_ids.clear();
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
    getPIDContext(pkt.getPID())->packets++;
    _demux.feedPacket(pkt);
}


//----------------------------------------------------------------------------
// Get the characteristics of the PID's.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::getPIDs(PIDSet& pids) const
{
    pids.reset();
    for (auto it = _pids.begin(); it != _pids.end(); ++it) {
        if (it->second->packets > 0) {
            pids.set(it->first);
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

bool ts::SignalizationDemux::inService(PID pid, uint16_t service_id) const
{
    auto ctx = _pids.find(pid);
    return ctx != _pids.end() && Contains(ctx->second->services, service_id);
}

bool ts::SignalizationDemux::inAnyService(PID pid, std::set<uint16_t> service_ids) const
{
    auto ctx = _pids.find(pid);
    if (ctx != _pids.end()) {
        for (auto it = service_ids.begin(); it != service_ids.end(); ++it) {
            if (Contains(ctx->second->services, *it)) {
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
// Get the context for a PID.
//----------------------------------------------------------------------------

ts::SignalizationDemux::PIDContextPtr ts::SignalizationDemux::getPIDContext(PID pid)
{
    auto it = _pids.find(pid);
    return it != _pids.end() ? it->second : (_pids[pid] = PIDContextPtr(new PIDContext(pid)));
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
    for (auto it = tids.begin(); it != tids.end(); ++it) {
        addFilteredTableId(*it);
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
                for (auto it = _last_pat.pmts.begin(); it != _last_pat.pmts.end(); ++it) {
                    _demux.addPID(it->second);
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
        default: {
            // Unsupported table id.
            return false;
        }
    }

    // Add the table id.
    _tids.insert(tid);
    return true;
}


//----------------------------------------------------------------------------
// Remove a signalization table id to filter.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeFilteredTableIds(std::initializer_list<TID> tids)
{
    for (auto it = tids.begin(); it != tids.end(); ++it) {
        removeFilteredTableId(*it);
    }
}

bool ts::SignalizationDemux::removeFilteredTableId(TID tid)
{
    // Do nothing if the table id was not filtered.
    if (!isFilteredTableId(tid)) {
        return false;
    }

    // Remove the table id first.
    _tids.erase(tid);

    // Configure the demux according to the table id.
    switch (tid) {
        case TID_PAT: {
            // Stop monitoring the PAT only when there is no need to get PMT's or NIT.
            if (!isFilteredTableId(TID_PMT) && _service_ids.empty() && !isFilteredTableId(TID_NIT_ACT) && !isFilteredTableId(TID_NIT_OTH)) {
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
                for (auto it = _last_pat.pmts.begin(); it != _last_pat.pmts.end(); ++it) {
                    if (!isFilteredServiceId(it->first)) {
                        _demux.removePID(it->second);
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
        default: {
            // Unsupported table id.
            return false;
        }
    }

    // Table id successfully removed.
    return true;
}


//----------------------------------------------------------------------------
// Add a service id to filter its PMT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::addFilteredServiceId(uint16_t sid)
{
    // Do something only when the service is not yet monitored.
    if (!isFilteredServiceId(sid)) {

        // Remember the service id to monitor.
        _service_ids.insert(sid);

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
// Remove a service id to filter its PMT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeFilteredServiceId(uint16_t sid)
{
    // Do something only when the service is currently monitored.
    if (isFilteredServiceId(sid)) {

        // Forget the service id to monitor.
        _service_ids.erase(sid);

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
// Remove all service ids to filter PMT's.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::removeAllFilteredServiceIds()
{
    // If a PAT is known, remove all PMT PID's.
    // If all PMT's are still monitored, don't change anything.
    if (_last_pat.isValid() && !isFilteredTableId(TID_PMT)) {
        for (auto it = _last_pat.pmts.begin(); it != _last_pat.pmts.end(); ++it) {
            _demux.removePID(it->second);
        }
    }

    // Forget all service ids.
    _service_ids.clear();
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
            }
            break;
        }
        case TID_MGT: {
            const MGT mgt(_duck, table);
            if (mgt.isValid() && pid == PID_PSIP && _handler != nullptr && isFilteredTableId(tid)) {
                _handler->handleMGT(mgt, pid);
            }
            break;
        }
        case TID_CVCT: {
            const CVCT vct(_duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct, pid);
                if (_handler != nullptr && isFilteredTableId(tid)) {
                    // Call specific and generic form of VCT handler.
                    _handler->handleCVCT(vct, pid);
                    _handler->handleVCT(vct, pid);
                }
            }
            break;
        }
        case TID_TVCT: {
            const TVCT vct(_duck, table);
            if (vct.isValid() && pid == PID_PSIP) {
                handleVCT(vct, pid);
                if (_handler != nullptr && isFilteredTableId(tid)) {
                    // Call specific and generic form of VCT handler.
                    _handler->handleTVCT(vct, pid);
                    _handler->handleVCT(vct, pid);
                }
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
    if (_last_pat.isValid() && (all_pmts || !_service_ids.empty())) {
        // Loop on all previous services
        for (auto it1 = _last_pat.pmts.begin(); it1 != _last_pat.pmts.end(); ++it1) {
            // If the service no longer exists or has changed its PMT PID, remove the previous PMT PID.
            const auto it2(pat.pmts.find(it1->first));
            if (it2 == pat.pmts.end() || it2->second != it1->second) {
                _demux.removePID(it1->second);
            }
        }
    }

    // Remove all services which were in a previous PAT and no longer here.
    for (auto srv_it = _services.begin(); srv_it != _services.end(); ) {
        if (!srv_it->hasId()) {
            // Service without an id, move to next one.
            ++srv_it;
        }
        else {
            auto pat_it = pat.pmts.find(srv_it->getId());
            if (pat_it == pat.pmts.end()) {
                // Service has an id which is not in the PAT => remove from the service list.
                srv_it = _services.erase(srv_it);
            }
            else {
                // Keep the service, move to next one.
                srv_it->setPMTPID(pat_it->second);
                ++srv_it;
            }
        }
    }

    // Add services which were missing from the service list.
    for (auto pat_it = pat.pmts.begin(); pat_it != pat.pmts.end(); ++pat_it) {
        bool found = false;
        for (auto srv_it = _services.begin(); !found && srv_it != _services.end(); ++srv_it) {
            found = srv_it->hasId(pat_it->first);
        }
        if (!found) {
            auto srv_it = _services.emplace(_services.end(), pat_it->first);
            srv_it->setPMTPID(pat_it->second);
        }
    }

    // Remember the last PAT.
    _last_pat = pat;
    _last_pat_handled = false;
    _ts_id = pat.ts_id;

    // Then, monitor new PMT PID's. Some of them may be already monitored.
    for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
        if (all_pmts || isFilteredServiceId(it->first)) {
            _demux.addPID(it->second);
        }
    }

    // Monitor non-standard NIT PID.
    if (isFilteredTableId(TID_NIT_ACT) || isFilteredTableId(TID_NIT_OTH)) {
        _demux.addPID(nitPID());
    }

    // Notify the PAT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_PAT)) {
        _last_pat_handled = true;
        _handler->handlePAT(pat, pid);
    }

    // Reprocess the last NIT in case of PAT change (TS id may have changed).
    if (_last_nit.isValid()) {
        handleNIT(_last_nit, nitPID());
    }
}


//----------------------------------------------------------------------------
// Process a CAT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleCAT(const CAT& cat, PID pid)
{
    // Look for EMM PID's in the CAT.
    handleDescriptors(cat.descs, pid);

    // Notify the CAT to the application.
    if (_handler != nullptr && isFilteredTableId(TID_CAT)) {
        _handler->handleCAT(cat, pid);
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handlePMT(const PMT& pmt, PID pid)
{
    // In case of PMT update for an existing service, remove all previous PID's for this service.
    for (auto it = _pids.begin(); it != _pids.end(); ++it) {
        it->second->services.erase(pmt.service_id);
    }

    // Register the PMT PID as PSI.
    auto ctx(getPIDContext(pid));
    ctx->pid_class = PIDClass::PSI;
    ctx->services.insert(pmt.service_id);

    // Look for ECM PID's at service level.
    handleDescriptors(pmt.descs, pid);

    // Loop on all components.
    for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {

        // Register the characteritics of the component PID.
        ctx = getPIDContext(it->first);
        ctx->pid_class = it->second.getClass(_duck);
        ctx->stream_type = it->second.stream_type;
        ctx->codec = it->second.getCodec(_duck);
        ctx->services.insert(pmt.service_id);

        // Look for ECM PID's at component level.
        handleDescriptors(pmt.descs, pid);
    }

    // Notify the PMT to the application.
    if (_handler != nullptr && (isFilteredTableId(TID_PMT) || isFilteredServiceId(pmt.service_id))) {
        _handler->handlePMT(pmt, pid);
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
            _last_nit_handled = false;
        }

        // If there is no PAT, we don't know the TS id, reprocess later.
        if (!_last_pat.isValid()) {
            return;
        }

        // Process services (LCN).
        //@@@
    }

    // Notify the NIT to the application.
    if (_handler != nullptr && isFilteredTableId(nit.tableId())) {
        _last_nit_handled = _last_nit_handled || nit.isActual();
        _handler->handleNIT(nit, pid);
    }
}


//----------------------------------------------------------------------------
// Process an SDT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleSDT(const SDT& sdt, PID pid)
{
    // Extract information on this TS only on the SDT Actual.
    if (sdt.isActual()) {
        _orig_network_id = sdt.onetw_id;
        //@@@
    }

    // Notify the NIT to the application.
    if (_handler != nullptr && isFilteredTableId(sdt.tableId())) {
        _handler->handleSDT(sdt, pid);
    }
}


//----------------------------------------------------------------------------
// Process a TVCT or CVCT.
//----------------------------------------------------------------------------

void ts::SignalizationDemux::handleVCT(const VCT& vct, PID pid)
{
    //@@@
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
            else if ((_duck.standards() & Standards::ISDB) != Standards::NONE && did == DID_ISDB_CA) {
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

// Constructor.
ts::SignalizationDemux::PIDContext::PIDContext(PID pid_) :
    pid(pid_),
    pid_class(PIDClass::UNDEFINED),
    codec(CodecType::UNDEFINED),
    stream_type(ST_NULL),
    cas_id(CASID_NULL),
    packets(0),
    services()
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

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

#include "tsSignalizationDemux.h"
#include "tsBinaryTable.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SignalizationDemux::SignalizationDemux(DuckContext& duck, SignalizationHandlerInterface* handler, std::initializer_list<TID> tids) :
    _duck(duck),
    _demux(duck, this, this),
    _handler(handler),
    _tids(),
    _service_ids(),
    _last_pat(),
    _last_pat_handled(false)
{
    _last_pat.invalidate();
    for (auto it = tids.begin(); it != tids.end(); ++it) {
        addTableId(*it);
    }
}


//----------------------------------------------------------------------------
// Get the NIT PID, either from last PAT or default PID.
//----------------------------------------------------------------------------

ts::PID ts::SignalizationDemux::nitPID() const
{
    return _last_pat.isValid() && _last_pat.nit_pid != PID_NULL ? _last_pat.nit_pid : PID(PID_NIT);
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
}


//----------------------------------------------------------------------------
// Add a signalization table id to filter.
//----------------------------------------------------------------------------

bool ts::SignalizationDemux::addTableId(TID tid)
{
    // Do not repeat already filtered table ids.
    if (hasTableId(tid)) {
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

bool ts::SignalizationDemux::removeTableId(TID tid)
{
    // Do nothing if the table id was not filtered.
    if (!hasTableId(tid)) {
        return false;
    }

    // Remove the table id first.
    _tids.erase(tid);

    // Configure the demux according to the table id.
    switch (tid) {
        case TID_PAT: {
            // Stop monitoring the PAT only when there is no need to get PMT's or NIT.
            if (!hasTableId(TID_PMT) && _service_ids.empty() && !hasTableId(TID_NIT_ACT) && !hasTableId(TID_NIT_OTH)) {
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
                    if (!hasServiceId(it->first)) {
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
            if (!hasTableId(TID_NIT_ACT) && !hasTableId(TID_NIT_OTH)) {
                _demux.removePID(nitPID());
            }
            break;
        }
        case TID_SDT_ACT:
        case TID_SDT_OTH:
        case TID_BAT: {
            // SDT and BAT share the same PID. Remove the PID only if none is monitored.
            if (!hasTableId(TID_SDT_ACT) && !hasTableId(TID_SDT_OTH) && !hasTableId(TID_BAT)) {
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
            if (!hasTableId(TID_TDT) && !hasTableId(TID_TOT)) {
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
            if (!hasTableId(TID_MGT) && !hasTableId(TID_CVCT) && !hasTableId(TID_TVCT) && !hasTableId(TID_RRT) && !hasTableId(TID_STT)) {
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

void ts::SignalizationDemux::addServiceId(uint16_t sid)
{
    // Do something only when the service is not yet monitored.
    if (!hasServiceId(sid)) {

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

void ts::SignalizationDemux::removeServiceId(uint16_t sid)
{
    // Do something only when the service is currently monitored.
    if (hasServiceId(sid)) {

        // Forget the service id to monitor.
        _service_ids.erase(sid);

        // If a PAT is known and references the service, remove its PMT PID.
        // If all PMT's are still monitored, don't change anything.
        if (_last_pat.isValid() && !hasTableId(TID_PMT)) {
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

void ts::SignalizationDemux::removeAllServiceIds()
{
    // If a PAT is known, remove all PMT PID's.
    // If all PMT's are still monitored, don't change anything.
    if (_last_pat.isValid() && !hasTableId(TID_PMT)) {
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
    const PID pid = table.sourcePID();
    const TID tid = table.tableId();

    // The PAT needs to be monitored outside explicit filtering.
    if (tid == TID_PAT && pid == PID_PAT) {
        const PAT pat(_duck, table);
        if (pat.isValid()) {

            // Check if all PMT's are monitored.
            const bool all_pmts = hasTableId(TID_PMT);

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

            // Remember the last PAT.
            _last_pat = pat;
            _last_pat_handled = false;

            // Then, monitor new PMT PID's. Some of them may be already monitored.
            for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                if (all_pmts || hasServiceId(it->first)) {
                    _demux.addPID(it->second);
                }
            }

            // Monitor non-standard NIT PID.
            if (hasTableId(TID_NIT_ACT) || hasTableId(TID_NIT_OTH)) {
                _demux.addPID(nitPID());
            }

            // Notify the PAT to the application.
            if (_handler != nullptr && hasTableId(TID_PAT)) {
                _last_pat_handled = true;
                _handler->handlePAT(pat, pid);
            }
        }
    }

    // Other tables have no special treatment. They are directly passed to the application.
    // PMT may be selectively filtered by service id (table id extention).
    else if (_handler != nullptr && (hasTableId(tid) || (tid == TID_PMT && hasServiceId(table.tableIdExtension())))) {
        switch (tid) {
            case TID_CAT: {
                const CAT cat(_duck, table);
                if (cat.isValid() && pid == PID_CAT) {
                    _handler->handleCAT(cat, pid);
                }
                break;
            }
            case TID_PMT: {
                const PMT pmt(_duck, table);
                if (pmt.isValid()) {
                    _handler->handlePMT(pmt, pid);
                }
                break;
            }
            case TID_TSDT: {
                const TSDT tsdt(_duck, table);
                if (tsdt.isValid() && pid == PID_TSDT) {
                    _handler->handleTSDT(tsdt, pid);
                }
                break;
            }
            case TID_NIT_ACT:
            case TID_NIT_OTH:  {
                const NIT nit(_duck, table);
                if (nit.isValid() && pid == nitPID()) {
                    _handler->handleNIT(nit, pid);
                }
                break;
            }
            case TID_SDT_ACT:
            case TID_SDT_OTH:  {
                const SDT sdt(_duck, table);
                if (sdt.isValid() && pid == PID_SDT) {
                    _handler->handleSDT(sdt, pid);
                }
                break;
            }
            case TID_BAT: {
                const BAT bat(_duck, table);
                if (bat.isValid() && pid == PID_BAT) {
                    _handler->handleBAT(bat, pid);
                }
                break;
            }
            case TID_RST: {
                const RST rst(_duck, table);
                if (rst.isValid() && pid == PID_RST) {
                    _handler->handleRST(rst, pid);
                }
                break;
            }
            case TID_TDT: {
                const TDT tdt(_duck, table);
                if (tdt.isValid() && pid == PID_TDT) {
                    _handler->handleTDT(tdt, pid);
                }
                break;
            }
            case TID_TOT: {
                const TOT tot(_duck, table);
                if (tot.isValid() && pid == PID_TOT) {
                    _handler->handleTOT(tot, pid);
                }
                break;
            }
            case TID_MGT: {
                const MGT mgt(_duck, table);
                if (mgt.isValid() && pid == PID_PSIP) {
                    _handler->handleMGT(mgt, pid);
                }
                break;
            }
            case TID_CVCT: {
                const CVCT vct(_duck, table);
                if (vct.isValid() && pid == PID_PSIP) {
                    // Call specific and generic form of VCT handler.
                    _handler->handleCVCT(vct, pid);
                    _handler->handleVCT(vct, pid);
                }
                break;
            }
            case TID_TVCT: {
                const TVCT vct(_duck, table);
                if (vct.isValid() && pid == PID_PSIP) {
                    // Call specific and generic form of VCT handler.
                    _handler->handleTVCT(vct, pid);
                    _handler->handleVCT(vct, pid);
                }
                break;
            }
            case TID_RRT: {
                const RRT rrt(_duck, table);
                if (rrt.isValid() && pid == PID_PSIP) {
                    _handler->handleRRT(rrt, pid);
                }
                break;
            }
            default: {
                // Unsupported table id or processed elsewhere (PAT, STT).
                break;
            }
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
    if (_handler != nullptr && section.tableId() == TID_STT && hasTableId(TID_STT) && section.sourcePID() == PID_PSIP) {
        const STT stt(_duck, section);
        if (stt.isValid()) {
            _handler->handleSTT(stt, PID_PSIP);
        }
    }
}

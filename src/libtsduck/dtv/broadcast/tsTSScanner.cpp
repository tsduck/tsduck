//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsTSScanner.h"
#include "tsBinaryTable.h"
#include "tsTime.h"
#include "tsTSPacket.h"
#include "tsTVCT.h"
#include "tsCVCT.h"
#include "tsLogicalChannelNumbers.h"

#define BUFFER_PACKET_COUNT  10000 // packets


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSScanner::TSScanner(DuckContext& duck, Tuner& tuner, MilliSecond timeout, bool pat_only):
    _duck(duck),
    _pat_only(pat_only),
    _completed(false),
    _demux(_duck, this),
    _tparams(),
    _pat(),
    _sdt(),
    _nit(),
    _mgt(),
    _vct()
{
    // Collect PAT, SDT, NIT, MGT.
    _demux.addPID(PID_PAT);
    if (!_pat_only) {
        _demux.addPID(PID_SDT);
        _demux.addPID(PID_NIT);
        _demux.addPID(PID_PSIP);
    }

    // Start packet acquisition
    if (!tuner.start()) {
        return;
    }

    // Deadline for table collection
    const Time deadline(timeout == Infinite ? Time::Apocalypse : Time::CurrentUTC() + timeout);

    // Allocate packet buffer on heap (risk of stack overflow)
    std::vector<TSPacket> buffer(BUFFER_PACKET_COUNT);

    // Read packets and analyze tables until completed
    while (!_completed && Time::CurrentUTC() < deadline) {
        const size_t pcount = tuner.receive(buffer.data(), buffer.size(), nullptr);
        _duck.report().debug(u"got %d packets", {pcount});
        if (pcount == 0) { // error
            break;
        }
        for (size_t n = 0; !_completed && n < pcount; ++n) {
            _demux.feedPacket(buffer[n]);
        }
    }

    // Get current tuning parameters before finishing.
    // Some tuners may take a while to update their internal status.
    if (!tuner.getCurrentTuning(_tparams, true)) {
        _tparams.clear();
    }

    // Stop packet acquisition
    tuner.stop();
}


//----------------------------------------------------------------------------
// Get the list of services.
//----------------------------------------------------------------------------

bool ts::TSScanner::getServices(ServiceList& services) const
{
    services.clear();

    if (_pat.isNull()) {
        _duck.report().warning(u"No PAT found, services are unknown");
        return false;
    }

    if (_sdt.isNull() && _vct.isNull() && !_pat_only) {
        _duck.report().warning(u"No SDT or VCT found, services names are unknown");
        // do not return, collect service ids.
    }

    // Loop on all services in the PAT
    for (auto it = _pat->pmts.begin(); it != _pat->pmts.end(); ++it) {

        // Service id, PMT PID and TS id are extracted from the PAT
        Service srv;
        srv.setId(it->first);
        srv.setPMTPID(it->second);
        srv.setTSId(_pat->ts_id);

        // Original netw. id, service type, name and provider are extracted from the SDT.
        if (!_sdt.isNull()) {
            srv.setONId(_sdt->onetw_id);
            // Search service in the SDT
            const auto sit = _sdt->services.find(srv.getId());
            if (sit != _sdt->services.end()) {
                const uint8_t type = sit->second.serviceType(_duck);
                const UString name(sit->second.serviceName(_duck));
                const UString provider(sit->second.providerName(_duck));
                if (type != 0) {
                    srv.setTypeDVB(type);
                }
                if (!name.empty()) {
                    srv.setName(name);
                }
                if (!provider.empty()) {
                    srv.setProvider(provider);
                }
                srv.setCAControlled(sit->second.CA_controlled);
                srv.setEITpfPresent(sit->second.EITpf_present);
                srv.setEITsPresent(sit->second.EITs_present);
                srv.setRunningStatus(sit->second.running_status);
            }
        }

        // ATSC service descriptions are extracted from the VCT.
        if (!_vct.isNull()) {
            // Search service in the VCT
            const auto sit = _vct->findService(srv.getId());
            if (sit != _vct->channels.end()) {
                if (sit->second.service_type != 0) {
                    srv.setTypeATSC(sit->second.service_type);
                }
                if (!sit->second.short_name.empty()) {
                    srv.setName(sit->second.short_name);
                }
                srv.setCAControlled(sit->second.access_controlled);
                if (sit->second.major_channel_number > 0) {
                    // Major channel numbers start at 1.
                    srv.setMajorIdATSC(sit->second.major_channel_number);
                }
                // Minor channel number 0 is valid (means analog).
                srv.setMinorIdATSC(sit->second.minor_channel_number);
            }
        }

        // Add new service definition in result
        services.push_back(srv);
    }

    // Logical channel numbers are extracted from the NIT.
    if (!_nit.isNull()) {
        LogicalChannelNumbers lcn_store(_duck);
        lcn_store.addFromNIT(*_nit);
        lcn_store.updateServices(services, true, false);
    }

    return true;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::TSScanner::handleTable(SectionDemux&, const BinaryTable& table)
{
    _duck.report().debug(u"got table id 0x%X on PID 0x%X", {table.tableId(), table.sourcePID()});

    // Store known tables
    switch (table.tableId()) {

        case TID_PAT: {
            SafePtr<PAT> pat(new PAT(_duck, table));
            if (pat->isValid()) {
                _pat = pat;
                if (_pat->nit_pid != PID_NULL && _pat->nit_pid != PID_NIT) {
                    // Non standard NIT PID
                    _demux.removePID(PID_NIT);
                    _demux.addPID(pat->nit_pid);
                }
            }
            break;
        }

        case TID_SDT_ACT: {
            SafePtr<SDT> sdt(new SDT(_duck, table));
            if (sdt->isValid()) {
                _sdt = sdt;
            }
            break;
        }

        case TID_NIT_ACT: {
            SafePtr<NIT> nit(new NIT(_duck, table));
            if (nit->isValid()) {
                _nit = nit;
            }
            break;
        }

        case TID_MGT: {
            SafePtr<MGT> mgt(new MGT(_duck, table));
            if (mgt->isValid()) {
                _mgt = mgt;
                // Intercept TVCT and CVCT, they contain the service names.
                for (auto it = mgt->tables.begin(); it != mgt->tables.end(); ++it) {
                    switch (it->second.table_type) {
                        case ATSC_TTYPE_TVCT_CURRENT:
                        case ATSC_TTYPE_CVCT_CURRENT:
                            _demux.addPID(it->second.table_type_PID);
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }

        case TID_TVCT: {
            SafePtr<VCT> vct(new TVCT(_duck, table));
            if (vct->isValid()) {
                _vct = vct;
            }
            break;
        }

        case TID_CVCT: {
            SafePtr<VCT> vct(new CVCT(_duck, table));
            if (vct->isValid()) {
                _vct = vct;
            }
            break;
        }

        default: {
            break;
        }
    }

    // When all tables are ready, stop collection
    _completed = !_pat.isNull() && (_pat_only || (!_sdt.isNull() && !_nit.isNull()) || (!_mgt.isNull() && !_vct.isNull()));
}

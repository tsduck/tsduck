//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSScanner.h"
#include "tsBinaryTable.h"
#include "tsTime.h"
#include "tsTSPacket.h"
#include "tsPMT.h"
#include "tsTVCT.h"
#include "tsCVCT.h"
#include "tsATSC.h"

#define BUFFER_PACKET_COUNT  10000 // packets


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSScanner::TSScanner(DuckContext& duck, Tuner& tuner, cn::milliseconds timeout, bool pat_only):
    _duck(duck),
    _pat_only(pat_only)
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
    const Time deadline(Time::CurrentUTC() + timeout);

    // Allocate packet buffer on heap (risk of stack overflow)
    std::vector<TSPacket> buffer(BUFFER_PACKET_COUNT);

    // Read packets and analyze tables until completed
    while (!_completed && Time::CurrentUTC() < deadline) {
        const size_t pcount = tuner.receive(buffer.data(), buffer.size(), nullptr);
        _duck.report().debug(u"got %d packets", pcount);
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

    if (_pat == nullptr) {
        _duck.report().warning(u"No PAT found, services are unknown");
        return false;
    }

    if (_sdt == nullptr && _vct == nullptr && !_pat_only) {
        _duck.report().warning(u"No SDT or VCT found, services names are unknown");
        // do not return, collect service ids.
    }

    // Loop on all services in the PAT
    for (const auto& srv_it : _pat->pmts) {

        // Service id, PMT PID and TS id are extracted from the PAT
        Service srv;
        srv.setId(srv_it.first);
        srv.setPMTPID(srv_it.second);
        srv.setTSId(_pat->ts_id);

        // Original netw. id, service type, name and provider are extracted from the SDT.
        if (_sdt != nullptr) {
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
        if (_vct != nullptr) {
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
                srv.setHidden(sit->second.hidden);
            }
        }

        // Add new service definition in result
        services.push_back(srv);
    }

    // Update logical channel numbers.
    _lcn.updateServices(services, Replacement::UPDATE);

    return true;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::TSScanner::handleTable(SectionDemux&, const BinaryTable& table)
{
    _duck.report().debug(u"got table id 0x%X on PID 0x%X", table.tableId(), table.sourcePID());

    // Store known tables
    switch (table.tableId()) {

        case TID_PAT: {
            std::shared_ptr<PAT> pat(new PAT(_duck, table));
            if (pat->isValid()) {
                _pat = std::move(pat);
                // Collect the NIT.
                if (_pat->nit_pid != PID_NULL && _pat->nit_pid != PID_NIT) {
                    // Non standard NIT PID
                    _demux.removePID(PID_NIT);
                    _demux.addPID(_pat->nit_pid);
                }
                // Collect all PMT's (to later collect elementary streams carrying sections).
                for (const auto& it : _pat->pmts) {
                    _demux.addPID(it.second);
                }
            }
            break;
        }

        case TID_PMT: {
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                // Collect tables on PID's carrying sections.
                // In practice, this is required only for Astra SGT which contains LCN (stream type 0x05).
                for (const auto& it : pmt.streams) {
                    if (it.second.stream_type == ST_PRIV_SECT) {
                        _demux.addPID(it.first);
                    }
                }
            }
            break;
        }

        case TID_SDT_ACT: {
            std::shared_ptr<SDT> sdt(new SDT(_duck, table));
            if (sdt->isValid()) {
                _sdt = std::move(sdt);
            }
            break;
        }

        case TID_NIT_ACT: {
            std::shared_ptr<NIT> nit(new NIT(_duck, table));
            if (nit->isValid()) {
                _nit = std::move(nit);
                _lcn.addFromNIT(*_nit);
            }
            break;
        }

        case TID_MGT: {
            std::shared_ptr<MGT> mgt(new MGT(_duck, table));
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
            std::shared_ptr<VCT> vct(new TVCT(_duck, table));
            if (vct->isValid()) {
                _vct = std::move(vct);
            }
            break;
        }

        case TID_CVCT: {
            std::shared_ptr<VCT> vct(new CVCT(_duck, table));
            if (vct->isValid()) {
                _vct = std::move(vct);
            }
            break;
        }

        case TID_ASTRA_SGT: {
            const SGT sgt(_duck, table);
            if (sgt.isValid()) {
                // Collect logical channel numbers.
                _lcn.addFromSGT(sgt);
            }
            break;
        }

        default: {
            break;
        }
    }

    // When all tables are ready, stop collection
    _completed = _pat != nullptr && (_pat_only || (_sdt != nullptr && _nit != nullptr) || (_mgt != nullptr && _vct != nullptr));
}

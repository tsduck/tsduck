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
//  A class which scans the services of a transport stream.
//
//----------------------------------------------------------------------------

#include "tsTSScanner.h"
#include "tsTime.h"
TSDUCK_SOURCE;

#define BUFFER_PACKET_COUNT  10000 // packets


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSScanner::TSScanner(Tuner& tuner, MilliSecond timeout, bool pat_only, Report& report):
    _pat_only(pat_only),
    _completed(false),
    _report(report),
    _demux(this),
    _tparams(),
    _pat(),
    _sdt(),
    _nit()
{
    // Collect PAT, SDT, NIT
    _demux.addPID(PID_PAT);
    if (!_pat_only) {
        _demux.addPID(PID_SDT);
        _demux.addPID(PID_NIT);
    }

    // Start packet acquisition
    if (!tuner.start(_report)) {
        return;
    }

    // Get current tuning parameters.
    _tparams = TunerParameters::Factory(tuner.tunerType());
    if (!_tparams.isNull() && !tuner.getCurrentTuning(*_tparams, true, _report)) {
        _tparams.clear();
    }

    // Deadline for table collection
    const Time deadline(timeout == Infinite ? Time::Apocalypse : Time::CurrentUTC() + timeout);

    // Allocate packet buffer on heap (risk of stack overflow)
    std::vector<TSPacket> buffer(BUFFER_PACKET_COUNT);

    // Read packets and analyze tables until completed
    while (!_completed && Time::CurrentUTC() < deadline) {
        const size_t pcount = tuner.receive(buffer.data(), buffer.size(), nullptr, _report);
        _report.debug(u"got %d packets", {pcount});
        if (pcount == 0) { // error
            break;
        }
        for (size_t n = 0; !_completed && n < pcount; ++n) {
            _demux.feedPacket(buffer[n]);
        }
    }

    // Stop packet acquisition
    tuner.stop(_report);
}


//----------------------------------------------------------------------------
// Get the list of services.
//----------------------------------------------------------------------------

bool ts::TSScanner::getServices(ServiceList& services) const
{
    services.clear();

    if (_pat.isNull()) {
        _report.warning(u"No PAT found, services are unknown");
        return false;
    }

    if (_sdt.isNull() && !_pat_only) {
        _report.warning(u"No SDT found, services names are unknown");
        // do not return, collect service ids.
    }

    // Loop on all services in the PAT
    for (PAT::ServiceMap::const_iterator it = _pat->pmts.begin(); it != _pat->pmts.end(); ++it) {

        // Service id, PMT PID and TS id are extracted from the PAT
        Service srv;
        srv.setId(it->first);
        srv.setPMTPID(it->second);
        srv.setTSId(_pat->ts_id);

        // Original netw. id, service type, name and provider are extracted from the SDT.
        if (!_sdt.isNull()) {
            srv.setONId(_sdt->onetw_id);
            // Search service in the SDT
            const SDT::ServiceMap::const_iterator sit = _sdt->services.find(srv.getId());
            if (sit != _sdt->services.end()) {
                const uint8_t type = sit->second.serviceType();
                const UString name(sit->second.serviceName());
                const UString provider(sit->second.providerName());
                if (type != 0) {
                    srv.setType(type);
                }
                if (!name.empty()) {
                    srv.setName(name);
                }
                if (!provider.empty()) {
                    srv.setProvider(provider);
                }
            }
        }

        // Logical channel number is extracted from the NIT.
        // Since locating the TS in the NIT requires the ONId, the SDT must be there as well.
        if (!_nit.isNull() && !_sdt.isNull()) {
            // Search the TS in the NIT
            const TransportStreamId ts(srv.getTSId(), srv.getONId());
            const NIT::TransportMap::const_iterator tit = _nit->transports.find(ts);
            if (tit != _nit->transports.end()) {
                const DescriptorList& dlist(tit->second.descs);
                // Loop on all logical_channel_number_descriptors
                for (size_t i = dlist.search(DID_LOGICAL_CHANNEL_NUM, 0, PDS_EICTA);
                     i < dlist.count() && !srv.hasLCN();
                     i = dlist.search(DID_LOGICAL_CHANNEL_NUM, i + 1, PDS_EICTA)) {

                    const uint8_t* data = dlist[i]->payload();
                    size_t size = dlist[i]->payloadSize();
                    while (size >= 4 && !srv.hasLCN()) {
                        if (GetUInt16(data) == srv.getId()) {
                            srv.setLCN(GetUInt16(data + 2) & 0x03FF);
                        }
                        data += 4;
                        size -= 4;
                    }
                }
            }
        }

        // Add new service definition in result
        services.push_back(srv);
    }

    return true;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::TSScanner::handleTable(SectionDemux&, const BinaryTable& table)
{
    _report.debug(u"got table id 0x%X on PID 0x%X", {table.tableId(), table.sourcePID()});

    // Store known tables
    switch (table.tableId()) {

        case TID_PAT: {
            SafePtr<PAT> pat(new PAT(table));
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
            SafePtr<SDT> sdt(new SDT(table));
            if (sdt->isValid()) {
                _sdt = sdt;
            }
            break;
        }

        case TID_NIT_ACT: {
            SafePtr<NIT> nit(new NIT(table));
            if (nit->isValid()) {
                _nit = nit;
            }
            break;
        }

        default: {
            break;
        }
    }

    // When all tables are ready, stop collection
    _completed = !_pat.isNull() && (_pat_only || (!_sdt.isNull() && !_nit.isNull()));
}

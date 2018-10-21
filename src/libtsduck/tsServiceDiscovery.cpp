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

#include "tsServiceDiscovery.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::ServiceDiscovery::ServiceDiscovery(PMTHandlerInterface* pmtHandler, Report& report, const DVBCharset* charset) :
    Service(),
    _report(report),
    _notFound(false),
    _charset(charset),
    _pmtHandler(pmtHandler),
    _pmt(),
    _demux(this)
{
    _pmt.invalidate();
}


//----------------------------------------------------------------------------
// Constructor using a string description.
//----------------------------------------------------------------------------

ts::ServiceDiscovery::ServiceDiscovery(const UString& desc, PMTHandlerInterface* pmtHandler, Report& report, const DVBCharset* charset) :
    ServiceDiscovery(pmtHandler, report, charset)
{
    set(desc);
}


//----------------------------------------------------------------------------
// Reset using a string description.
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::set(const UString& desc)
{
    // Clear and set superclass.
    Service::set(desc);

    // Start to intercept tables.
    if (hasName()) {
        // We know the service name, get SDT first, PAT later.
        _demux.addPID(PID_SDT);
    }
    else if (hasId()) {
        // We know the service id, get PAT and SDT.
        _demux.addPID(PID_PAT);
        _demux.addPID(PID_SDT);
    }
    else {
        // We have neither name nor id (desc was an empty string).
        // Get the PAT and we will select the first service within it.
        _demux.addPID(PID_PAT);
    }
}


//----------------------------------------------------------------------------
// Clear all fields
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::clear()
{
    _demux.reset();
    _pmt.invalidate();
    Service::clear();
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::handleTable(SectionDemux& demux, const BinaryTable& table)
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
            if (pmt.isValid() && hasId(pmt.service_id)) {
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
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processSDT(const SDT& sdt)
{
    // Look for the service by name or by service
    uint16_t service_id = 0;
    SDT::ServiceMap::const_iterator srv = sdt.services.end();

    if (!hasName()) {
        // Service is known by id only.
        assert(hasId());
        service_id = getId();
        srv = sdt.services.find(service_id);
        if (srv == sdt.services.end()) {
            // Service not referenced in the SDT, not a problem.
            return;
        }
    }
    else if (sdt.findService(getName(), service_id)) {
        // Service is found by name in the SDT.
        srv = sdt.services.find(service_id);
        assert(srv != sdt.services.end());
    }
    else {
        // Service not found by name in SDT. If we already know the service id, this is fine.
        // If we do not know the service id, then there is no way to find the service.
        if (!hasId()) {
            _report.error(u"service \"%s\" not found in SDT", {getName()});
            _notFound = true;
        }
        return;
    }

    // If the service id was previously unknown wait for the PAT.
    // If a service id was known but was different, we need to rescan the PAT.
    if (!hasId(service_id)) {
        if (hasId()) {
            // The service was previously known but has changed its service id.
            // We need to rescan the service map. The PMT is reset.
            if (hasPMTPID()) {
                _demux.removePID(getPMTPID());
            }
            _pmt.invalidate();
        }

        // We now know the service id (or new service id).
        setId(service_id);

        // But we do not know yet the PMT PID, we must (re)scan the PAT for this.
        clearPMTPID();
        _demux.resetPID(PID_PAT);
        _demux.addPID(PID_PAT);

        _report.verbose(u"found service \"%s\", service id is 0x%X (%d)", {getName(), getId(), getId()});
    }

    // Now collect suitable information from the SDT.
    setTSId(sdt.ts_id);
    setONId(sdt.onetw_id);
    setCAControlled(srv->second.CA_controlled);
    setEITpfPresent(srv->second.EITpf_present);
    setEITsPresent(srv->second.EITs_present);
    setRunningStatus(srv->second.running_status);
    setType(srv->second.serviceType());
    setName(srv->second.serviceName(_charset));
    setProvider(srv->second.providerName(_charset));
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processPAT(const PAT& pat)
{
    // Locate the service in the PAT.
    PAT::ServiceMap::const_iterator it = pat.pmts.end();
    if (hasId()) {
        // A service id was known, locate the service in the PAT.
        it = pat.pmts.find(getId());
        if (it == pat.pmts.end()) {
            _report.error(u"service id 0x%X (%d) not found in PAT", {getId(), getId()});
            _notFound = true;
            return;
        }
    }
    else {
        // If no service was specified, use the first service from the PAT.
        if (pat.pmts.empty()) {
            _report.error(u"no service found in PAT");
            _notFound = true;
            return;
        }
        it = pat.pmts.begin();
        // Now, we have a service id.
        setId(it->first);
        // Intercept the SDT for more details.
        _demux.addPID(PID_SDT);
    }

    // If the PMT PID was previously unknown wait for the PMT.
    // If the PMT PID was known but was different, we need to rescan the PMT.
    if (!hasPMTPID(it->second)) {
        // Store new PMT PID.
        setPMTPID(it->second);

        // (Re)scan the PMT.
        _demux.resetPID(it->second);
        _demux.addPID(it->second);

        // Invalidate out PMT.
        _pmt.invalidate();

        _report.verbose(u"found service id 0x%X (%d), PMT PID is 0x%X (%d)", {getId(), getId(), getPMTPID(), getPMTPID()});
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processPMT(const PMT& pmt)
{
    // Store the new PMT.
    _pmt = pmt;

    // Notify the application.
    if (_pmtHandler != nullptr) {
        _pmtHandler->handlePMT(_pmt);
    }
}

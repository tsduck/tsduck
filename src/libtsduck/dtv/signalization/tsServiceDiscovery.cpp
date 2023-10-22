//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceDiscovery.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsSDT.h"
#include "tsMGT.h"
#include "tsCVCT.h"
#include "tsTVCT.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::ServiceDiscovery::ServiceDiscovery(DuckContext& duck, SignalizationHandlerInterface* pmtHandler) :
    _duck(duck),
    _pmtHandler(pmtHandler)
{
    _pmt.invalidate();
}


//----------------------------------------------------------------------------
// Constructor using a string description.
//----------------------------------------------------------------------------

ts::ServiceDiscovery::ServiceDiscovery(DuckContext& duck, const UString& desc, SignalizationHandlerInterface* pmtHandler) :
    ServiceDiscovery(duck, pmtHandler)
{
    set(desc);
}


//----------------------------------------------------------------------------
// Reset using a string description.
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::set(const UString& desc)
{
    // Clear and set superclass.
    // In discovery mode (this subclass), "-" means first service in the PAT, same as an empty string.
    Service::set(desc == u"-" ? UString() : desc);

    // Start to intercept tables.
    if (hasName()) {
        // We know the service name, get SDT (DVB) or xVCT (ATSC) first, PAT later.
        _demux.addPID(PID_SDT);
        _demux.addPID(PID_PSIP);
    }
    else if (hasId()) {
        // We know the service id, get PAT and SDT or xVCT.
        _demux.addPID(PID_PAT);
        _demux.addPID(PID_SDT);
        _demux.addPID(PID_PSIP);
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
                PAT pat(_duck, table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }
        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(_duck, table);
                if (sdt.isValid()) {
                    processSDT(sdt);
                }
            }
            break;
        }
        case TID_MGT: {
            MGT mgt(_duck, table);
            if (mgt.isValid()) {
                analyzeMGT(mgt);
            }
            break;
        }
        case TID_TVCT: {
            TVCT tvct(_duck, table);
            if (tvct.isValid()) {
                analyzeVCT(tvct);
            }
            break;
        }
        case TID_CVCT: {
            CVCT cvct(_duck, table);
            if (cvct.isValid()) {
                analyzeVCT(cvct);
            }
            break;
        }
        case TID_PMT: {
            PMT pmt(_duck, table);
            if (pmt.isValid() && hasId(pmt.service_id)) {
                processPMT(pmt, table.sourcePID());
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// This method processes a Service Description Table (SDT).
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processSDT(const SDT& sdt)
{
    // Look for the service by name or by service
    uint16_t service_id = 0;
    auto srv = sdt.services.end();

    if (!hasName()) {
        // Service is known by id only.
        assert(hasId());
        service_id = getId();
        srv = sdt.services.find(service_id);
        if (srv == sdt.services.end()) {
            // Service not referenced in the SDT, not a problem, we already know the service id.
            return;
        }
    }
    else if (sdt.findService(_duck, getName(), service_id)) {
        // Service is found by name in the SDT.
        srv = sdt.services.find(service_id);
        assert(srv != sdt.services.end());
    }
    else {
        // Service not found by name in SDT. If we already know the service id, this is fine.
        // If we do not know the service id, then there is no way to find the service.
        if (!hasId()) {
            _duck.report().error(u"service \"%s\" not found in SDT", {getName()});
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

        _duck.report().verbose(u"found service \"%s\", service id is 0x%X (%d)", {getName(), getId(), getId()});
    }

    // Now collect suitable information from the SDT.
    setTSId(sdt.ts_id);
    setONId(sdt.onetw_id);
    setCAControlled(srv->second.CA_controlled);
    setEITpfPresent(srv->second.EITpf_present);
    setEITsPresent(srv->second.EITs_present);
    setRunningStatus(srv->second.running_status);
    setTypeDVB(srv->second.serviceType(_duck));
    setName(srv->second.serviceName(_duck));
    setProvider(srv->second.providerName(_duck));
}


//----------------------------------------------------------------------------
// This method processes an ATSC Master Guide Table (MGT)
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::analyzeMGT(const MGT& mgt)
{
    // Process all table types.
    for (const auto& it : mgt.tables) {

        // Intercept TVCT and CVCT, they contain the service names.
        switch (it.second.table_type) {
            case ATSC_TTYPE_TVCT_CURRENT:
            case ATSC_TTYPE_CVCT_CURRENT:
                _demux.addPID(it.second.table_type_PID);
                break;
            default:
                break;
        }
    }
}


//----------------------------------------------------------------------------
// This method processes ATSC Terrestrial or Cable Virtual Channel Table.
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::analyzeVCT(const VCT& vct)
{
    // Look for the service by name or by service
    auto srv = vct.channels.end();

    if (!hasName()) {
        // Service is known by id only.
        assert(hasId());
        srv = vct.findService(getId());
        if (srv == vct.channels.end()) {
            // Service not referenced in the VCT, not a problem, we already know the service id.
            return;
        }
    }
    else if ((srv = vct.findService(getName())) == vct.channels.end()) {
        // Service not found by name in VCT. If we already know the service id, this is fine.
        // If we do not know the service id, then there is no way to find the service.
        if (!hasId()) {
            _duck.report().error(u"service \"%s\" not found in VCT", {getName()});
            _notFound = true;
        }
        return;
    }

    // If the service id was previously unknown wait for the PAT.
    // If a service id was known but was different, we need to rescan the PAT.
    assert(srv != vct.channels.end());
    if (!hasId(srv->second.program_number)) {
        if (hasId()) {
            // The service was previously known but has changed its service id.
            // We need to rescan the service map. The PMT is reset.
            if (hasPMTPID()) {
                _demux.removePID(getPMTPID());
            }
            _pmt.invalidate();
        }

        // We now know the service id (or new service id).
        setId(srv->second.program_number);

        // But we do not know yet the PMT PID, we must (re)scan the PAT for this.
        clearPMTPID();
        _demux.resetPID(PID_PAT);
        _demux.addPID(PID_PAT);

        _duck.report().verbose(u"found service \"%s\", service id is 0x%X (%d)", {getName(), getId(), getId()});
    }

    // Now collect suitable information from the VCT.
    srv->second.updateService(*this);
}


//----------------------------------------------------------------------------
// This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processPAT(const PAT& pat)
{
    // Locate the service in the PAT.
    auto it = pat.pmts.end();
    if (hasId()) {
        // A service id was known, locate the service in the PAT.
        it = pat.pmts.find(getId());
        if (it == pat.pmts.end()) {
            _duck.report().error(u"service id 0x%X (%d) not found in PAT", {getId(), getId()});
            _notFound = true;
            return;
        }
    }
    else {
        // If no service was specified, use the first service from the PAT.
        if (pat.pmts.empty()) {
            _duck.report().error(u"no service found in PAT");
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

        _duck.report().verbose(u"found service id 0x%X (%d), PMT PID is 0x%X (%d)", {getId(), getId(), getPMTPID(), getPMTPID()});
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::ServiceDiscovery::processPMT(const PMT& pmt, PID pid)
{
    // Store the new PMT.
    _pmt = pmt;

    // Notify the application.
    if (_pmtHandler != nullptr) {
        _pmtHandler->handlePMT(_pmt, pid);
    }
}

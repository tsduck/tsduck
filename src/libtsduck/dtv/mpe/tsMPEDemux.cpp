//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEDemux.h"
#include "tsMPEPacket.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsIPMACStreamLocationDescriptor.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::MPEDemux::MPEDemux(DuckContext& duck, MPEHandlerInterface* mpe_handler, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter),
    _handler(mpe_handler),
    _psi_demux(duck, this, this)
{
    immediateReset();
}

ts::MPEDemux::~MPEDemux()
{
}


//----------------------------------------------------------------------------
// Add / remove MPE PID's (overridden from AbstractDemux).
// The added / removed PID's are also added / removed in the section demux.
//----------------------------------------------------------------------------

void ts::MPEDemux::addPID(PID pid)
{
    SuperClass::addPID(pid);
    _psi_demux.addPID(pid);
}

void ts::MPEDemux::addPIDs(const PIDSet& pids)
{
    SuperClass::addPIDs(pids);
    _psi_demux.addPIDs(pids);
}

void ts::MPEDemux::removePID(PID pid)
{
    SuperClass::removePID(pid);
    _psi_demux.removePID(pid);
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built PES packets).
//----------------------------------------------------------------------------

void ts::MPEDemux::immediateReset()
{
    SuperClass::immediateReset();

    // Reset the PSI demux since the transport may be completely different.
    _psi_demux.reset();

    // Forget everything about the current TS.
    _ts_id = 0;
    _pmts.clear();
    _new_pids.reset();
    _int_tags.clear();

    // To get PID's with MPE, we need to analyze the PMT's.
    // To get the PMT PID's, we need to analyze the PAT.
    _psi_demux.addPID(PID_PAT);
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::MPEDemux::feedPacket(const TSPacket& pkt)
{
    // Super class processing first.
    SuperClass::feedPacket(pkt);

    // Submit the packet to the PSI handler to detect MPE streams.
    _psi_demux.feedPacket(pkt);
}


//----------------------------------------------------------------------------
// Invoked by the PSI demux when a single section is available.
// Used to collect DSM-CC sections carrying MPE.
//----------------------------------------------------------------------------

void ts::MPEDemux::handleSection(SectionDemux& demux, const Section& section)
{
    // We are notified of absolutely all sections, including PMT, etc.
    // So, we need to carefully filter the sections. This must be a
    // DSM-CC Private Data section and it must come from a PID we filter.

    if (section.tableId() == TID_DSMCC_PD && _pid_filter.test(section.sourcePID())) {

        // Build the corresponding MPE packet.
        MPEPacket mpe(section);
        if (mpe.isValid() && _handler != nullptr) {

            // Send the MPE packet to the application.
            beforeCallingHandler(section.sourcePID());
            try {
                _handler->handleMPEPacket(*this, mpe);
            }
            catch (...) {
                afterCallingHandler(false);
                throw;
            }
            afterCallingHandler(true);
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the PSI demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::MPEDemux::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(_duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                // Remember our transport stream.
                _ts_id = pat.ts_id;
                // Add all PMT PID's to PSI demux.
                for (const auto& it : pat.pmts) {
                    _psi_demux.addPID(it.second);
                }
            }
            break;
        }

        case TID_PMT: {
            PMTPtr pmt(new PMT(_duck, table));
            if (!pmt.isNull() && pmt->isValid()) {
                // Keep track of all PMT's in the TS.
                _pmts[pmt->service_id] = pmt;
                // Process content of the PMT.
                processPMT(*pmt);
            }
            break;
        }

        case TID_INT: {
            INT imnt(_duck, table);
            if (imnt.isValid()) {
                processINT(imnt);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::MPEDemux::processPMT(const PMT& pmt)
{
    // Loop on all components of the service.
    for (const auto& it : pmt.streams) {

        const PID pid = it.first;
        const PMT::Stream& stream(it.second);

        // Loop on all data_broadcast_id_descriptors for the component.
        for (size_t i = stream.descs.search(DID_DATA_BROADCAST_ID); i < stream.descs.count(); i = stream.descs.search(DID_DATA_BROADCAST_ID, i + 1)) {
            if (!stream.descs[i].isNull()) {
                const DataBroadcastIdDescriptor desc(_duck, *stream.descs[i]);
                if (desc.isValid()) {
                    // Found a valid data_broadcast_id_descriptor.
                    switch (desc.data_broadcast_id) {
                        case DBID_IPMAC_NOTIFICATION:
                            // This component carries INT tables.
                            // We need to collect the INT.
                            _psi_demux.addPID(pid);
                            break;
                        case DBID_MPE:
                            // This component carries MPE sections.
                            processMPEDiscovery(pmt, pid);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        // Look for an optional stream_identifier_descriptor for this component.
        uint8_t ctag = 0;
        if (stream.getComponentTag(ctag) && _int_tags.count(ServiceTagToInt(pmt.service_id, ctag)) != 0) {
            // This PID was signalled as MPE in the INT, process it.
            processMPEDiscovery(pmt, pid);
        }
    }
}


//----------------------------------------------------------------------------
// Process an INT (IP/MAC Notification Table).
//----------------------------------------------------------------------------

void ts::MPEDemux::processINT(const INT& imnt)
{
    // Process all descriptor lists in the table. Normally, the IP/MAC stream
    // location descriptors should be only in the operational descriptor loop
    // of a device. But we should be prepared to incorrect signalization.

    processINTDescriptors(imnt.platform_descs);
    for (const auto& it : imnt.devices) {
        processINTDescriptors(it.second.target_descs);
        processINTDescriptors(it.second.operational_descs);
    }
}


//----------------------------------------------------------------------------
// Process a descriptor list in the INT.
//----------------------------------------------------------------------------

void ts::MPEDemux::processINTDescriptors(const DescriptorList& descs)
{
    // Loop on all IP/MAC stream_location_descriptors.
    for (size_t i = descs.search(DID_INT_STREAM_LOC); i < descs.count(); i = descs.search(DID_INT_STREAM_LOC, i + 1)) {
        const IPMACStreamLocationDescriptor desc(_duck, *descs[i]);
        if (desc.isValid() && desc.transport_stream_id == _ts_id) {
            // Found an MPE PID in this transport stream.

            // First, record the MPE service and component.
            _int_tags.insert(ServiceTagToInt(desc.service_id, desc.component_tag));

            // Check if we already found the PMT for this service
            const auto it = _pmts.find(desc.service_id);
            PID pid = PID_NULL;
            if (it != _pmts.end() && (pid = it->second->componentTagToPID(desc.component_tag)) != PID_NULL) {
                // Yes, the PMT was already found and it has a component with the specified tag.
                processMPEDiscovery(*it->second, pid);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process the discovery of a new MPE PID.
//----------------------------------------------------------------------------

void ts::MPEDemux::processMPEDiscovery(const PMT& pmt, PID pid)
{
    // Don't signal the same PID twice.
    if (!_new_pids.test(pid) && _handler != nullptr) {

        // Remember we signalled this PID.
        _new_pids.set(pid);

        // Invoke the user-defined handler to signal the new PID.
        beforeCallingHandler(pid);
        try {
            _handler->handleMPENewPID(*this, pmt, pid);
        }
        catch (...) {
            afterCallingHandler(false);
            throw;
        }
        afterCallingHandler(true);
    }
}

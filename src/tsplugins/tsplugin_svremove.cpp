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
//
//  Transport stream processor shared library:
//  Remove a service
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsAlgorithm.h"
#include "tsNames.h"
#include "tsEITProcessor.h"
#include "tsCADescriptor.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsNIT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SVRemovePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(SVRemovePlugin);
    public:
        // Implementation of plugin API
        SVRemovePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool              _abort;          // Error (service not found, etc)
        bool              _ready;          // Ready to pass packets
        bool              _transparent;    // Transparent mode, pass all packets
        Service           _service;        // Service name & id
        bool              _ignore_absent;  // Ignore service if absent
        bool              _ignore_bat;     // Do not modify the BAT
        bool              _ignore_eit;     // Do not modify the EIT's
        bool              _ignore_nit;     // Do not modify the NIT
        Status            _drop_status;    // Status for dropped packets
        PIDSet            _drop_pids;      // List of PIDs to drop
        PIDSet            _ref_pids;       // List of other referenced PIDs
        SectionDemux      _demux;          // Section demux
        CyclingPacketizer _pzer_pat;       // Packetizer for modified PAT
        CyclingPacketizer _pzer_sdt_bat;   // Packetizer for modified SDT/BAT
        CyclingPacketizer _pzer_nit;       // Packetizer for modified NIT
        EITProcessor      _eit_process;    // Modify EIT's

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables and descriptors
        void processPAT(PAT&);
        void processSDT(SDT&);
        void processPMT(PMT&);
        void processNITBAT(AbstractTransportListTable&);
        void processNITBATDescriptorList(DescriptorList&);

        // Mark all ECM PIDs from the specified descriptor list in the specified PID set
        void addECMPID(const DescriptorList&, PIDSet&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"svremove", ts::SVRemovePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SVRemovePlugin::SVRemovePlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove a service", u"[options] service"),
    _abort(false),
    _ready(false),
    _transparent(false),
    _service(),
    _ignore_absent(false),
    _ignore_bat(false),
    _ignore_eit(false),
    _ignore_nit(false),
    _drop_status(TSP_DROP),
    _drop_pids(),
    _ref_pids(),
    _demux(duck, this),
    _pzer_pat(duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _pzer_sdt_bat(duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _pzer_nit(duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _eit_process(duck, PID_EIT)
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specifies the service to remove. If the argument is an integer value "
         u"(either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored.");

    option(u"ignore-absent", 'a');
    help(u"ignore-absent",
         u"Ignore service if not present in the transport stream. By default, tsp "
         u"fails if the service is not found.");

    option(u"ignore-bat", 'b');
    help(u"ignore-bat", u"Do not modify the BAT.");

    option(u"ignore-eit", 'e');
    help(u"ignore-eit", u"Do not modify the EIT's.");

    option(u"ignore-nit", 'n');
    help(u"ignore-nit", u"Do not modify the NIT.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SVRemovePlugin::start()
{
    // Get option values
    duck.loadArgs(*this);
    _service.set(value(u""));
    _ignore_absent = present(u"ignore-absent");
    _ignore_bat = present(u"ignore-bat");
    _ignore_eit = present(u"ignore-eit");
    _ignore_nit = present(u"ignore-nit");
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    // Initialize the demux
    _demux.reset();
    _demux.addPID(PID_SDT);

    // When the service id is known, we wait for the PAT. If it is not yet
    // known (only the service name is known), we do not know how to modify
    // the PAT. We will wait for it after receiving the SDT.
    // Packets from PAT PID are analyzed but not passed. When a complete
    // PAT is read, a modified PAT will be transmitted.
    if (_service.hasId()) {
        _demux.addPID(PID_PAT);
        if (!_ignore_nit) {
            _demux.addPID(PID_NIT);
        }
    }

    // Initialize the EIT processing.
    _eit_process.reset();

    // Build a list of referenced PID's (except those in the removed service).
    // Prevent predefined PID's from being removed.
    _ref_pids.reset();
    _ref_pids.set(PID_PAT);
    _ref_pids.set(PID_CAT);
    _ref_pids.set(PID_TSDT);
    _ref_pids.set(PID_NULL);  // keep stuffing as well
    _ref_pids.set(PID_NIT);
    _ref_pids.set(PID_SDT);   // also contains BAT
    _ref_pids.set(PID_EIT);
    _ref_pids.set(PID_RST);
    _ref_pids.set(PID_TDT);   // also contains TOT
    _ref_pids.set(PID_NETSYNC);
    _ref_pids.set(PID_RNT);
    _ref_pids.set(PID_INBSIGN);
    _ref_pids.set(PID_MEASURE);
    _ref_pids.set(PID_DIT);
    _ref_pids.set(PID_SIT);

    // Reset other states
    _abort = false;
    _ready = false;
    _transparent = false;
    _drop_pids.reset();
    _pzer_pat.reset();
    _pzer_sdt_bat.reset();
    _pzer_nit.reset();

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (tsp->debug()) {
        tsp->debug(u"Got %s v%d, PID %d (0x%X), TIDext %d (0x%X)",
                   {names::TID(duck, table.tableId()), table.version(),
                    table.sourcePID(), table.sourcePID(),
                    table.tableIdExtension(), table.tableIdExtension()});
    }

    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(duck, table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(duck, table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }

        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(duck, table);
                if (sdt.isValid()) {
                    processSDT(sdt);
                }
            }
            break;
        }

        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                // SDT Other are passed unmodified
                _pzer_sdt_bat.removeSections(TID_SDT_OTH, table.tableIdExtension());
                _pzer_sdt_bat.addTable(table);
            }
            break;
        }

        case TID_BAT:
            if (table.sourcePID() == PID_BAT) {
                if (!_service.hasId()) {
                    // The BAT and SDT are on the same PID. Here, we are in the case
                    // were the service was designated by name and the first BAT arrives
                    // before the first SDT. We do not know yet how to modify the BAT.
                    // Reset the demux on this PID, so that this BAT will be submitted
                    // again the next time.
                    _demux.resetPID(table.sourcePID());
                }
                else if (_ignore_bat) {
                    // Do not modify BAT
                    _pzer_sdt_bat.removeSections(TID_BAT, table.tableIdExtension());
                    _pzer_sdt_bat.addTable(table);
                }
                else {
                    // Modify BAT
                    BAT bat(duck, table);
                    if (bat.isValid()) {
                        processNITBAT(bat);
                        _pzer_sdt_bat.removeSections(TID_BAT, bat.bouquet_id);
                        _pzer_sdt_bat.addTable(duck, bat);
                    }
                }
            }
            break;

        case TID_NIT_ACT: {
            if (table.sourcePID() == PID_NIT) {
                if (_ignore_nit) {
                    // Do not modify NIT Actual
                    _pzer_nit.removeSections(TID_NIT_ACT, table.tableIdExtension());
                    _pzer_nit.addTable(table);
                }
                else {
                    // Modify NIT Actual
                    NIT nit(duck, table);
                    if (nit.isValid()) {
                        processNITBAT(nit);
                        _pzer_nit.removeSections(TID_NIT_ACT, nit.network_id);
                        _pzer_nit.addTable(duck, nit);
                    }
                }
            }
            break;
        }

        case TID_NIT_OTH: {
            if (table.sourcePID() == PID_NIT) {
                // NIT Other are passed unmodified
                _pzer_nit.removeSections(TID_NIT_OTH, table.tableIdExtension());
                _pzer_nit.addTable(table);
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

void ts::SVRemovePlugin::processSDT(SDT& sdt)
{
    bool found = false;

    // Look for the service by name or by id
    if (_service.hasId()) {
        // Search service by id
        found = Contains(sdt.services, _service.getId());
        if (!found) {
            // Informational only, SDT entry is not mandatory.
            tsp->info(u"service %d (0x%X) not found in SDT, ignoring it", {_service.getId(), _service.getId()});
        }
    }
    else {
        // Service id is currently unknown, search service by name
        found = sdt.findService(duck, _service);
        if (!found) {
            // Here, this is an error. A service can be searched by name only in current TS
            if (_ignore_absent) {
                tsp->warning(u"service \"%s\" not found in SDT, ignoring it", {_service.getName()});
                _transparent = true;
            }
            else {
                tsp->error(u"service \"%s\" not found in SDT", {_service.getName()});
                _abort = true;
            }
            return;
        }
        // The service id was previously unknown, now wait for the PAT
        _demux.addPID(PID_PAT);
        if (!_ignore_nit) {
            _demux.addPID(PID_NIT);
        }
        tsp->verbose(u"found service \"%s\", service id is 0x%X", {_service.getName(), _service.getId()});
    }

    // Remove service description in the SDT
    if (_service.hasId()) {
        sdt.services.erase(_service.getId());
    }

    // Replace the SDT in the PID
    _pzer_sdt_bat.removeSections(TID_SDT_ACT, sdt.ts_id);
    _pzer_sdt_bat.addTable(duck, sdt);
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::processPAT(PAT& pat)
{
    // PAT not normally fetched until service id is known
    assert(_service.hasId());

    // Save the NIT PID
    _pzer_nit.setPID(pat.nit_pid);
    _demux.addPID(pat.nit_pid);

    // Loop on all services in the PAT. We need to scan all PMT's to know which
    // PID to remove and which to keep (if shared between the removed service
    // and other services).
    bool found = false;
    for (const auto& it : pat.pmts) {
        // Scan all PMT's
        _demux.addPID(it.second);

        // Check if service to remove is here
        if (it.first == _service.getId()) {
            found = true;
            _service.setPMTPID(it.second);
            tsp->verbose(u"found service id 0x%X (%<d), PMT PID is 0x%X (%<d)", {_service.getId(), _service.getPMTPID()});
            // Drop PMT of the service
            _drop_pids.set(it.second);
        }
        else {
            // Mark other PMT's as referenced
            _ref_pids.set(it.second);
        }
    }

    if (found) {
        // Remove the service from the PAT
        pat.pmts.erase(_service.getId());
    }
    else if (_ignore_absent || !_ignore_nit || !_ignore_bat) {
        // Service is not present in current TS, but continue
        tsp->info(u"service id 0x%X not found in PAT, ignoring it", {_service.getId()});
        _ready = true;
    }
    else {
        // If service is not found and no need to modify to NIT or BAT, abort
        tsp->error(u"service id 0x%X not found in PAT", {_service.getId()});
        _abort = true;
    }

    // Replace the PAT.in the PID
    _pzer_pat.removeSections(TID_PAT);
    _pzer_pat.addTable(duck, pat);

    // Remove EIT's for this service.
    if (!_ignore_eit) {
        _eit_process.removeService(_service);
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::processPMT(PMT& pmt)
{
    // Is this the PMT of the service to remove?
    const bool removed_service = pmt.service_id == _service.getId();

    // Mark PIDs as dropped or referenced.
    PIDSet& pid_set(removed_service ? _drop_pids : _ref_pids);

    // Mark all program-level ECM PID's
    addECMPID(pmt.descs, pid_set);

    // Mark service's PCR PID (usually a referenced component or null PID)
    pid_set.set(pmt.pcr_pid);

    // Loop on all elementary streams
    for (const auto& it : pmt.streams) {
        // Mark component's PID
        pid_set.set(it.first);
        // Mark all component-level ECM PID's
        addECMPID(it.second.descs, pid_set);
    }

    // When the service to remove has been analyzed, we are ready to filter PIDs
    _ready = _ready || removed_service;
}


//----------------------------------------------------------------------------
// Mark all ECM PIDs from the descriptor list in the PID set
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::addECMPID(const DescriptorList& dlist, PIDSet& pid_set)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {
        CADescriptor ca(duck, *dlist[index]);
        if (!ca.isValid()) {
            // Cannot deserialize a valid CA descriptor, ignore it
        }
        else {
            // Standard CAS, only one PID in CA descriptor
            pid_set.set(ca.ca_pid);
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a NIT or a BAT
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::processNITBAT(AbstractTransportListTable& table)
{
    // Process the global descriptor list
    processNITBATDescriptorList(table.descs);

    // Process each TS descriptor list
    for (auto& it : table.transports) {
        processNITBATDescriptorList(it.second.descs);
    }

    // No need to get the same section layout as input.
    table.clearPreferredSections();
}


//----------------------------------------------------------------------------
//  This method processes a NIT or a BAT descriptor list
//----------------------------------------------------------------------------

void ts::SVRemovePlugin::processNITBATDescriptorList(DescriptorList& dlist)
{
    // Process all service_list_descriptors
    for (size_t i = dlist.search (DID_SERVICE_LIST); i < dlist.count(); i = dlist.search (DID_SERVICE_LIST, i + 1)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();
        uint8_t* data = base;
        uint8_t* new_data = base;

        while (size >= 3) {
            if (GetUInt16 (data) != _service.getId()) {
                // Not the removed service, keep this entry
                new_data[0] = data[0];
                new_data[1] = data[1];
                new_data[2] = data[2];
                new_data += 3;
            }
            data += 3;
            size -= 3;
        }
        dlist[i]->resizePayload (new_data - base);
    }

    // Process all logical_channel_number_descriptors
    for (size_t i = dlist.search (DID_LOGICAL_CHANNEL_NUM, 0, PDS_EICTA);
         i < dlist.count();
         i = dlist.search (DID_LOGICAL_CHANNEL_NUM, i + 1, PDS_EICTA)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();
        uint8_t* data = base;
        uint8_t* new_data = base;

        while (size >= 4) {
            if (GetUInt16 (data) != _service.getId()) {
                // Not the removed service, keep this entry
                new_data[0] = data[0];
                new_data[1] = data[1];
                new_data[2] = data[2];
                new_data[3] = data[3];
                new_data += 4;
            }
            data += 4;
            size -= 4;
        }
        dlist[i]->resizePayload(new_data - base);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SVRemovePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Pass packets in transparent mode
    if (_transparent) {
        return TSP_OK;
    }

    // Filter interesting sections
    _demux.feedPacket(pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // As long as the original service-id or PMT are unknown, drop or nullify packets
    if (!_ready) {
        return _drop_status;
    }

    // Packets from removed PIDs are either dropped or nullified
    if (_drop_pids[pid] && !_ref_pids[pid]) {
        return _drop_status;
    }

    // Replace packets using packetizers
    if (pid == _pzer_pat.getPID()) {
        _pzer_pat.getNextPacket (pkt);
    }
    else if (pid == _pzer_sdt_bat.getPID()) {
        _pzer_sdt_bat.getNextPacket(pkt);
    }
    else if (!_ignore_nit && pid == _pzer_nit.getPID()) {
        _pzer_nit.getNextPacket(pkt);
    }
    else if (!_ignore_eit && pid == PID_EIT) {
        _eit_process.processPacket(pkt);
    }

    return TSP_OK;
}

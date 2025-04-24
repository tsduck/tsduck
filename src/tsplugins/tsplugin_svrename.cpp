//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Rename a service
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsNIT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SVRenamePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SVRenamePlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool              _abort = false;      // Error (service not found, etc)
        bool              _pat_found = false;  // PAT was found, ready to pass packets
        uint16_t          _ts_id = 0;          // Tranport stream id
        Service           _new_service {};     // New service name & id
        Service           _old_service {};     // Old service name & id
        bool              _ignore_bat = false; // Do not modify the BAT
        bool              _ignore_eit = false; // Do not modify the EIT's
        bool              _ignore_nit = false; // Do not modify the NIT
        SectionDemux      _demux {duck, this};
        CyclingPacketizer _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_pmt {duck, PID_NULL, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_sdt_bat {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_nit {duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor      _eit_process {duck, PID_EIT};

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables and descriptors
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);
        void processNITBAT(AbstractTransportListTable&);
        void processNITBATDescriptorList(DescriptorList&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"svrename", ts::SVRenamePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SVRenamePlugin::SVRenamePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Rename a service, assign a new service name and/or new service id", u"[options] [service]")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"",
         u"Specifies the service to rename. If the argument is an integer value "
         u"(either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. "
         u"If no service is specified, the first service in the PAT is used.");

    option(u"free-ca-mode", 'f', INTEGER, 0, 1, 0, 1);
    help(u"free-ca-mode", u"Specify a new free_CA_mode to set in the SDT (0 or 1).");

    option(u"id", 'i', UINT16);
    help(u"id", u"Specify a new service id value.");

    option(u"ignore-bat");
    help(u"ignore-bat", u"Do not modify the BAT.");

    option(u"ignore-eit");
    help(u"ignore-eit", u"Do not modify the EIT's.");

    option(u"ignore-nit");
    help(u"ignore-nit", u"Do not modify the NIT.");

    option(u"lcn", 'l', UINT16);
    help(u"lcn", u"Specify a new logical channel number (LCN).");

    option(u"name", 'n', STRING);
    help(u"name", u"string", u"Specify a new service name.");

    option(u"provider", 'p', STRING);
    help(u"provider", u"string", u"Specify a new provider name.");

    option(u"running-status", 'r', INTEGER, 0, 1, 0, 7);
    help(u"running-status", u"Specify a new running_status to set in the SDT (0 to 7).");

    option(u"type", 't', UINT8);
    help(u"type", u"Specify a new service type.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SVRenamePlugin::start()
{
    // Get option values
    duck.loadArgs(*this);
    _old_service.set(value(u""));
    _ignore_bat = present(u"ignore-bat");
    _ignore_eit = present(u"ignore-eit");
    _ignore_nit = present(u"ignore-nit");

    _new_service.clear();
    if (present(u"name")) {
        _new_service.setName(value(u"name"));
    }
    if (present(u"provider")) {
        _new_service.setProvider(value(u"provider"));
    }
    if (present(u"id")) {
        _new_service.setId(intValue<uint16_t>(u"id"));
    }
    if (present(u"lcn")) {
        _new_service.setLCN(intValue<uint16_t>(u"lcn"));
    }
    if (present(u"type")) {
        _new_service.setTypeDVB(intValue<uint8_t>(u"type"));
    }
    if (present(u"free-ca-mode")) {
        _new_service.setCAControlled(intValue<int>(u"free-ca-mode") != 0);
    }
    if (present(u"running-status")) {
        _new_service.setRunningStatus(intValue<uint8_t>(u"running-status"));
    }

    // Initialize the demux. When the service is unspecified or is known
    // by id, we wait for the PAT. If it is known by service name, we do
    // not know how to modify the PAT. We will wait for it after receiving the SDT.
    // Packets from PAT PID are analyzed but not passed. When a complete
    // PAT is read, a modified PAT will be transmitted.
    _demux.reset();
    _demux.addPID(_old_service.hasName() ? PID_SDT : PID_PAT);

    // Initialize the EIT processing.
    _eit_process.reset();

    // No need to modify EIT's if there is no new service id.
    if (!_new_service.hasId()) {
        _ignore_eit = true;
    }

    // Reset other states
    _abort = false;
    _pat_found = false;
    _ts_id = 0;
    _pzer_pat.reset();
    _pzer_pmt.reset();
    _pzer_sdt_bat.reset();
    _pzer_nit.reset();

    _pzer_pmt.setPID(PID_NULL);
    _pzer_nit.setPID(PID_NIT);

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (debug()) {
        debug(u"Got %s v%d, PID %n, TIDext %n",
              TIDName(duck, table.tableId(), table.sourcePID()), table.version(),
              table.sourcePID(), table.tableIdExtension());
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
            if (pmt.isValid() && _old_service.hasId(pmt.service_id)) {
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
                if (!_old_service.hasId()) {
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
            break;
        }

        case TID_NIT_OTH: {
            // NIT Other are passed unmodified
            _pzer_nit.removeSections(TID_NIT_OTH, table.tableIdExtension());
            _pzer_nit.addTable(table);
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Service Description Table (SDT).
//  We search the service in the SDT. Once we get the service, we rebuild a
//  new SDT containing only one section and only one service (a copy of
//  all descriptors for the service).
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::processSDT(SDT& sdt)
{
    bool found = false;

    // Save the TS id
    _ts_id = sdt.ts_id;

    // Look for the service by name or by service
    if (_old_service.hasId()) {
        // Search service by id. If the service is not present, this is not an error.
        found = sdt.services.contains(_old_service.getId());
        if (!found) {
            // Informational only
            verbose(u"service %n not found in SDT", _old_service.getId());
        }
    }
    else if (_old_service.hasName()) {
        // Search service by name only. The service id will be updated in _old_service.
        found = sdt.findService(duck, _old_service);
        if (!found) {
            // Here, this is an error. If the name is not in the SDT, then we cannot identify the service.
            error(u"service \"%s\" not found in SDT", _old_service.getName());
            _abort = true;
            return;
        }
        // The service id was previously unknown, now wait for the PAT
        _demux.addPID(PID_PAT);
        verbose(u"found service \"%s\", service id is 0x%X", _old_service.getName(), _old_service.getId());
    }

    // Modify the SDT with new service identification
    if (found) {
        if (_new_service.hasName()) {
            sdt.services[_old_service.getId()].setName(duck, _new_service.getName());
        }
        if (_new_service.hasProvider()) {
            sdt.services[_old_service.getId()].setProvider(duck, _new_service.getProvider());
        }
        if (_new_service.hasTypeDVB()) {
            sdt.services[_old_service.getId()].setType(_new_service.getTypeDVB());
        }
        if (_new_service.hasCAControlled()) {
            sdt.services[_old_service.getId()].CA_controlled = _new_service.getCAControlled();
        }
        if (_new_service.hasRunningStatus()) {
            sdt.services[_old_service.getId()].running_status = _new_service.getRunningStatus();
        }
        if (_new_service.hasId() && !_new_service.hasId(_old_service.getId())) {
            sdt.services[_new_service.getId()] = sdt.services[_old_service.getId()];
            sdt.services.erase(_old_service.getId());
        }
    }

    // Replace the SDT.in the PID
    _pzer_sdt_bat.removeSections(TID_SDT_ACT, sdt.ts_id);
    _pzer_sdt_bat.addTable(duck, sdt);
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::processPAT(PAT& pat)
{
    // Save the TS id.
    _ts_id = pat.ts_id;
    _old_service.setTSId(pat.ts_id);

    // Locate the service in the PAT.
    auto it = pat.pmts.end();
    if (_old_service.hasId()) {
        // The service id is known, find it in the PAT.
        it = pat.pmts.find(_old_service.getId());
    }
    else {
        // The service was originally unspecified, use the first service in the PAT.
        assert(!_old_service.hasName());
        if (pat.pmts.empty()) {
            error(u"the PAT contains no service");
            _abort = true;
            return;
        }
        it = pat.pmts.begin();
        _old_service.setId(it->first);
    }

    // If service not found, error
    if (it == pat.pmts.end()) {
        if (_ignore_nit && _ignore_bat && _ignore_eit) {
            error(u"service id 0x%X not found in PAT", _old_service.getId());
            _abort = true;
            return;
        }
        else {
            info(u"service id 0x%X not found in PAT, will still update NIT, BAT, EIT's", _old_service.getId());
        }
    }
    else {
        // Scan the PAT for the service
        _old_service.setPMTPID(it->second);
        _new_service.setPMTPID(it->second);
        _demux.addPID(it->second);
        _pzer_pmt.setPID(it->second);

        verbose(u"found service id 0x%X, PMT PID is 0x%X", _old_service.getId(), _old_service.getPMTPID());

        // Modify the PAT
        if (_new_service.hasId() && !_new_service.hasId(_old_service.getId())) {
            pat.pmts[_new_service.getId()] = pat.pmts[_old_service.getId()];
            pat.pmts.erase(_old_service.getId());
        }
    }

    // Replace the PAT in the PID.
    _pzer_pat.removeSections(TID_PAT);
    _pzer_pat.addTable(duck, pat);
    _pat_found = true;

    // Now that we know the ts_id, we can process the SDT and NIT.
    _demux.addPID(PID_SDT);
    if (!_ignore_nit) {
        const PID nit_pid = pat.nit_pid != PID_NULL ? pat.nit_pid : PID(PID_NIT);
        _pzer_nit.setPID(nit_pid);
        _demux.addPID(nit_pid);
    }

    // Rename the service in EIT's.
    if (!_ignore_eit) {
        _eit_process.renameService(_old_service, _new_service);
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::processPMT(PMT& pmt)
{
    // Change the service id in the PMT
    if (_new_service.hasId()) {
        pmt.service_id = _new_service.getId();
    }

    // Replace the PMT.in the PID
    _pzer_pmt.removeSections(TID_PMT, _old_service.getId());
    _pzer_pmt.removeSections(TID_PMT, _new_service.getId());
    _pzer_pmt.addTable(duck, pmt);
}


//----------------------------------------------------------------------------
//  This method processes a NIT or a BAT
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::processNITBAT(AbstractTransportListTable& table)
{
    // Process the descriptor list for the current TS
    for (auto& it : table.transports) {
        if (it.first.transport_stream_id == _ts_id) {
            processNITBATDescriptorList(it.second.descs);
        }
    }

    // No need to get the same section layout as input.
    table.clearPreferredSections();
}


//----------------------------------------------------------------------------
//  This method processes a NIT or a BAT descriptor list
//----------------------------------------------------------------------------

void ts::SVRenamePlugin::processNITBATDescriptorList(DescriptorList& dlist)
{
    // Process all service_list_descriptors
    for (size_t i = dlist.search(DID_DVB_SERVICE_LIST); i < dlist.count(); i = dlist.search(DID_DVB_SERVICE_LIST, i + 1)) {

        uint8_t* data = dlist[i].payload();
        size_t size = dlist[i].payloadSize();

        while (size >= 3) {
            uint16_t id = GetUInt16(data);
            if (id == _old_service.getId()) {
                if (_new_service.hasId()) {
                    PutUInt16(data, _new_service.getId());
                }
                if (_new_service.hasTypeDVB()) {
                    data[2] = _new_service.getTypeDVB();
                }
            }
            data += 3;
            size -= 3;
        }
    }

    // Process all logical_channel_number_descriptors
    for (size_t i = dlist.search(DID_EACEM_LCN, 0, PDS_EICTA);
         i < dlist.count();
         i = dlist.search(DID_EACEM_LCN, i + 1, PDS_EICTA)) {

        uint8_t* data = dlist[i].payload();
        size_t size = dlist[i].payloadSize();

        while (size >= 4) {
            uint16_t id = GetUInt16 (data);
            if (id == _old_service.getId()) {
                if (_new_service.hasId()) {
                    PutUInt16(data, _new_service.getId());
                }
                if (_new_service.hasLCN()) {
                    PutUInt16(data + 2, (GetUInt16(data + 2) & 0xFC00) | (_new_service.getLCN() & 0x03FF));
                }
            }
            data += 4;
            size -= 4;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SVRenamePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Filter interesting sections
    _demux.feedPacket(pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // As long as the original service-id or PMT are unknown, nullify packets
    if (!_pat_found) {
        return TSP_NULL;
    }

    // Replace packets using packetizers.
    if (pid != PID_NULL) {
        if (pid == PID_PAT) {
            _pzer_pat.getNextPacket(pkt);
        }
        else if (pid == PID_SDT) {
            _pzer_sdt_bat.getNextPacket(pkt);
        }
        else if (pid == _old_service.getPMTPID()) {
            _pzer_pmt.getNextPacket(pkt);
        }
        else if (!_ignore_nit && pid == _pzer_nit.getPID()) {
            _pzer_nit.getNextPacket(pkt);
        }
        else if (!_ignore_eit && pid == PID_EIT) {
            _eit_process.processPacket(pkt);
        }
    }

    return TSP_OK;
}

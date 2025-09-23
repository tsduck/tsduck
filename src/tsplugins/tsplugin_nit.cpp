//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Various transformations on the NIT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsNetworkNameDescriptor.h"
#include "tsServiceListDescriptor.h"
#include "tsNIT.h"
#include "tsPAT.h"
#include "tsSDT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(NITPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    protected:
        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

    private:
        // A map of service list descriptors, indexed by ts id / original netwok id.
        using SLDMap = std::map<TransportStreamId, ServiceListDescriptor>;

        PID                _nit_pid = PID_NIT;            // PID for the NIT (default: read PAT)
        UString            _new_netw_name {};             // New network name
        bool               _set_netw_id = false;          // Change network id
        uint16_t           _new_netw_id = 0;              // New network id
        bool               _set_onetw_id = false;         // Change original network id of all TS
        uint16_t           _new_onetw_id = 0;             // New original network id
        bool               _use_nit_other = false;        // Use a NIT Other, not the NIT Actual
        uint16_t           _nit_other_id = 0;             // Network id of the NIT Other to hack
        int                _lcn_oper = 0;                 // Operation on LCN descriptors
        int                _sld_oper = 0;                 // Operation on service_list_descriptors
        std::set<uint16_t> _remove_serv {};               // Set of services to remove
        std::set<uint16_t> _remove_ts {};                 // Set of transport streams to remove
        std::vector<DID>   _removed_desc {};              // Set of descriptor tags to remove
        PDS                _pds = 0;                      // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc = false;    // Remove private desc without preceding PDS desc
        bool               _update_mpe_fec = false;       // In terrestrial delivery
        uint8_t            _mpe_fec = 0;
        bool               _update_time_slicing = false;  // In terrestrial delivery
        uint8_t            _time_slicing = 0;
        bool               _build_sld = false;            // Build service list descriptors.
        bool               _add_all_srv_in_sld = false;   // Add all services in service list descriptors, even when the type in unknown.
        uint8_t            _default_srv_type = 0;         // Default service type in service list descriptors.
        SectionDemux       _demux {duck, this};           // Section demux to collect PAT and SDT to build service list descriptors.
        NIT                _last_nit {};                  // Last valid NIT found, after modification.
        PAT                _last_pat {};                  // Last valid input PAT.
        SDT                _last_sdt_act {};              // Last valid input SDT Actual.
        SLDMap             _collected_sld {};             // A map of service list descriptors per TS id.

        // Values for _lcn_oper and _sld_oper.
        enum {
            LCN_NONE          = 0,
            LCN_REMOVE        = 1,
            LCN_REMOVE_ODD    = 2,
            LCN_DUPLICATE_ODD = 3  // LCN only
        };

        // Process a list of descriptors according to the command line options.
        void processDescriptorList(DescriptorList&);

        // Merge last collected PAT in the collected services.
        // Return true if the list of collected services has been modified.
        bool mergeLastPAT();

        // Merge an SDTT in the collected services.
        // Return true if the list of collected services has been modified.
        bool mergeSDT(const SDT&);

        // Update the service list descriptors from collected services.
        void updateServiceList(NIT&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"nit", ts::NITPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NITPlugin::NITPlugin(TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the NIT", u"[options]", u"NIT", PID_NIT)
{
    option(u"build-service-list-descriptors", 0);
    help(u"build-service-list-descriptors",
         u"Build service_list_descriptors in the NIT according to the information which is "
         u"collected in the PAT and the SDT. See also option --default-service-type.");

    option(u"cleanup-private-descriptors", 0);
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

    option(u"default-service-type", 0, UINT8);
    help(u"default-service-type",
         u"With --build-service-list-descriptors, specify the default service type of "
         u"services which are found in the PAT but not in the SDT. "
         u"By default, services without known service type are not added in created "
         u"service list descriptors.");

    option(u"lcn", 'l', INTEGER, 0, 1, 1, 3);
    help(u"lcn",
         u"Specify which operation to perform on logical_channel_number (LCN) "
         u"descriptors. The value is a positive integer:\n"
         u"1: Remove all LCN descriptors.\n"
         u"2: Remove one entry every two entries in each LCN descriptor.\n"
         u"3: Duplicate one entry every two entries in each LCN descriptor.");

    option(u"mpe-fec", 0, INTEGER, 0, 1, 0, 1);
    help(u"mpe-fec",
         u"Set the \"MPE-FEC indicator\" in the terrestrial delivery system "
         u"descriptors to the specified value (0 or 1).");

    option(u"network-id", 0, UINT16);
    help(u"network-id", u"id",
         u"Set the specified new value as network id in the NIT.");

    option(u"network-name", 0, STRING);
    help(u"network-name", u"name",
         u"Set the specified value as network name in the NIT. Any existing network_name_descriptor "
         u"is removed. A new network_name_descriptor is created with the new name.");

    option(u"nit-other", 0, UINT16);
    help(u"nit-other", u"id",
         u"Same as --other (for compatibility).");

    option(u"original-network-id", 0, UINT16);
    help(u"original-network-id", u"id",
         u"Set the specified new value as original network id of all TS in the NIT.");

    option(u"other", 'o', UINT16);
    help(u"other", u"id",
         u"Do not modify the NIT Actual. Modify the NIT Other with the specified network id.");

    option(u"pds", 0, UINT32);
    help(u"pds",
         u"With option --remove-descriptor, specify the private data specifier "
         u"which applies to the descriptor tag values above 0x80.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID on which the NIT is expected. By default, use PID 16.");

    option(u"remove-descriptor", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"remove-descriptor",
         u"Remove from the NIT all descriptors with the specified tag. Several "
         u"--remove-descriptor options may be specified to remove several types of "
         u"descriptors. See also option --pds.");

    option(u"remove-service", 'r', UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-service",
         u"Remove the specified service_id from the following descriptors: "
         u"service_list_descriptor, logical_channel_number_descriptor. "
         u"Several --remove-service options may be specified to remove several "
         u"services.");

    option(u"remove-ts", 0, UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-ts",
         u"Remove the specified ts_id from the NIT. Several --remove-ts options "
         u"may be specified to remove several TS.");

    option(u"sld", 's', INTEGER, 0, 1, 1, 2);
    help(u"sld",
         u"Specify which operation to perform on service_list_descriptors. "
         u"The value is a positive integer:\n"
         u"1: Remove all service_list_descriptors.\n"
         u"2: Remove one entry every two entries in each descriptor.");

    option(u"time-slicing", 0, INTEGER, 0, 1, 0, 1);
    help(u"time-slicing",
         u"Set the \"time slicing indicator\" in the terrestrial delivery system "
         u"descriptors to the specified value (0 or 1).");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::NITPlugin::getOptions()
{
    getIntValue(_nit_pid, u"pid", PID_NULL);
    getIntValue(_lcn_oper, u"lcn", LCN_NONE);
    getIntValue(_sld_oper, u"sld", LCN_NONE);
    getIntValues(_remove_serv, u"remove-service");
    getIntValues(_remove_ts, u"remove-ts");
    getIntValues(_removed_desc, u"remove-descriptor");
    getIntValue(_pds, u"pds", 0);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    _update_mpe_fec = present(u"mpe-fec");
    _mpe_fec = intValue<uint8_t>(u"mpe-fec") & 0x01;
    _update_time_slicing = present(u"time-slicing");
    _time_slicing = intValue<uint8_t>(u"time-slicing") & 0x01;
    _new_netw_name = value(u"network-name");
    _set_netw_id = present(u"network-id");
    getIntValue(_new_netw_id, u"network-id");
    _set_onetw_id = present(u"original-network-id");
    getIntValue(_new_onetw_id, u"original-network-id");
    _use_nit_other = present(u"other") || present(u"nit-other");
    getIntValue(_nit_other_id, u"other", intValue<uint16_t>(u"nit-other"));
    _build_sld = present(u"build-service-list-descriptors");
    _add_all_srv_in_sld = present(u"default-service-type");
    getIntValue(_default_srv_type, u"default-service-type");

    if (_use_nit_other && _build_sld) {
        error(u"--nit-other and --build-service-list-descriptors are mutually exclusive");
        return false;
    }
    if (_lcn_oper != LCN_NONE && !_remove_serv.empty()) {
        error(u"--lcn and --remove-service are mutually exclusive");
        return false;
    }
    if (_sld_oper != LCN_NONE && !_remove_serv.empty()) {
        error(u"--sld and --remove-service are mutually exclusive");
        return false;
    }

    // Start superclass.
    return AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NITPlugin::start()
{
    // Reset state.
    _last_nit.invalidate();
    _last_pat.invalidate();
    _last_sdt_act.invalidate();
    _collected_sld.clear();

    // When we need to build service list descriptors, we need to analyze the PAT and SDT.
    _demux.reset();
    if (_build_sld && !_use_nit_other) {
        // If we need to add all services, including without known service type, analyze the PAT.
        if (_add_all_srv_in_sld) {
            _demux.addPID(PID_PAT);
        }
        // The service types are taken from the SDT.
        _demux.addPID(PID_SDT);
    }

    // Start superclass.
    return AbstractTablePlugin::start();
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::NITPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Filter incoming sections
    _demux.feedPacket(pkt);

    // Continue processing in superclass.
    return AbstractTablePlugin::processPacket(pkt, pkt_data);
}


//----------------------------------------------------------------------------
// Merge last collected PAT in the collected services.
//----------------------------------------------------------------------------

bool ts::NITPlugin::mergeLastPAT()
{
    bool modified = false;

    // To merge the services from the PAT, we need to know the original network id.
    // And we need the SDT Actual to know the original network id.
    if (_last_pat.isValid() && _last_sdt_act.isValid() && _add_all_srv_in_sld) {

        // Collected service list descriptor for this TS.
        const TransportStreamId tsid(_last_pat.ts_id, _last_sdt_act.onetw_id);
        ServiceListDescriptor& sld(_collected_sld[tsid]);

        // Loop on all services in the PAT.
        for (const auto& it : _last_pat.pmts) {
            if (!sld.hasService(it.first)) {
                modified = true;
                sld.entries.push_back(ServiceListDescriptor::Entry(it.first, _default_srv_type));
            }
        }

        // We no longer need the last collected PAT.
        _last_pat.invalidate();
    }

    return modified;
}


//----------------------------------------------------------------------------
// Merge an SDTT in the collected services.
//----------------------------------------------------------------------------

bool ts::NITPlugin::mergeSDT(const SDT& sdt)
{
    bool modified = false;

    // Remember last SDT Actual.
    if (sdt.isActual()) {
        _last_sdt_act = sdt;
        // The SDT Actual may allow the merge of the last PAT.
        modified = mergeLastPAT();
    }

    // Collected service list descriptor for this TS.
    const TransportStreamId tsid(sdt.ts_id, sdt.onetw_id);
    ServiceListDescriptor& sld(_collected_sld[tsid]);

    // Loop on all services in the SDT.
    for (const auto& it : sdt.services) {
        // Get service type in the SDT.
        uint8_t type = it.second.serviceType(duck);
        if (type == 0 && _add_all_srv_in_sld) {
            // Service type unknown in the SDT, use default service type.
            type = _default_srv_type;
        }
        if (type != 0) {
            // Update the service in the descriptor.
            modified = sld.addService(it.first, type) || modified;
        }
    }

    return modified;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::NITPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Analyze PAT and SDT when invoked from our demux.
    if (&demux == &_demux && !_use_nit_other) {

        const TID tid = table.tableId();
        const PID pid = table.sourcePID();
        bool modified = false;

        if (tid == TID_PAT && pid == PID_PAT && _add_all_srv_in_sld) {
            // Got a PAT, collect all service ids.
            const PAT pat(duck, table);
            if (pat.isValid()) {
                _last_pat = pat;
                modified = mergeLastPAT();
            }
        }
        else if ((tid == TID_SDT_ACT || tid == TID_SDT_OTH) && pid == PID_SDT) {
            // Got an SDT, collect service ids and types.
            const SDT sdt(duck, table);
            if (sdt.isValid()) {
                modified = mergeSDT(sdt);
            }
        }

        if (modified && _last_nit.isValid()) {
            // The global service list has been modified and a valid NIT was already found.
            updateServiceList(_last_nit);
            // Make sure the updated NIT has a new version.
            _last_nit.incrementVersion();
            // We need to force the modified NIT. Replace all if NIT Actual (only one instance possible).
            BinaryTable bin;
            _last_nit.serialize(duck, bin);
            forceTableUpdate(bin, _last_nit.isActual());
        }
    }

    // Call superclass.
    AbstractTablePlugin::handleTable(demux, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::NITPlugin::createNewTable(BinaryTable& table)
{
    NIT nit;

    // If we must modify one specific NIT Other, this is the one we need to create.
    if (_use_nit_other) {
        nit.setActual(false);
        nit.network_id = _nit_other_id;
    }

    nit.serialize(duck, table);

    // Keep track of last valid NIT.
    _last_nit = nit;
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::NITPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all)
{
    // If not the NIT we are looking for, reinsert without modification.
    is_target =
        (!_use_nit_other && table.tableId() == TID_NIT_ACT) ||
        (_use_nit_other && table.tableId() == TID_NIT_OTH && table.tableIdExtension() == _nit_other_id);
    if (!is_target) {
        return;
    }

    // Process the NIT.
    NIT nit(duck, table);
    if (!nit.isValid()) {
        warning(u"found invalid NIT");
        reinsert = false;
        return;
    }

    debug(u"got a NIT, version %d, network Id: %n", nit.version(), nit.network_id);

    // Replace all sections if NIT Actual (only one instance possible).
    replace_all = nit.isActual();

    // Remove the specified transport streams
    for (auto it = nit.transports.begin(); it != nit.transports.end(); ) {
        if (_remove_ts.count(it->first.transport_stream_id) != 0) {
            it = nit.transports.erase(it);
        }
        else {
            ++it;
        }
    }

    // Update the network id.
    if (_set_netw_id) {
        nit.network_id = _new_netw_id;
    }

    // Update the original network id of all TS.
    if (_set_onetw_id) {
        // The original network id is part of the map key (TransportStreamId).
        // First, build a set of keys with a different original network id.
        std::set<TransportStreamId> others;
        for (const auto& it : nit.transports) {
            if (it.first.original_network_id != _new_onetw_id) {
                others.insert(it.first);
            }
        }
        // Then, replace all keys.
        for (const auto& id : others) {
            auto newid(id);
            newid.original_network_id = _new_onetw_id;
            nit.transports[newid] = nit.transports[id];
            nit.transports.erase(id);
        }
    }

    // Update the network name.
    if (!_new_netw_name.empty()) {
        // Remove previous network_name_descriptor, if any.
        nit.descs.removeByTag(DID_DVB_NETWORK_NAME);
        // Add a new network_name_descriptor
        nit.descs.add(duck, NetworkNameDescriptor(_new_netw_name));
    }

    // Process the global descriptor list
    processDescriptorList(nit.descs);

    // Process each TS descriptor list
    for (auto& it : nit.transports) {
        processDescriptorList(it.second.descs);
    }

    // Update service list descriptors from collected services (if necessary).
    updateServiceList(nit);

    // Reserialize modified NIT.
    nit.clearPreferredSections();
    nit.serialize(duck, table);

    // Keep track of last valid NIT.
    _last_nit = nit;
}


//----------------------------------------------------------------------------
//  This method processes a NIT descriptor list
//----------------------------------------------------------------------------

void ts::NITPlugin::processDescriptorList(DescriptorList& dlist)
{
    // Process descriptor removal
    for (auto tag : _removed_desc) {
        dlist.removeByTag(tag, _pds);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        dlist.removeInvalidPrivateDescriptors();
    }

    // Process all terrestrial_delivery_system_descriptors
    for (size_t i = dlist.search(DID_DVB_TERREST_DELIVERY); i < dlist.count(); i = dlist.search(DID_DVB_TERREST_DELIVERY, i + 1)) {

        uint8_t* base = dlist[i].payload();
        size_t size = dlist[i].payloadSize();

        if (size > 4) {
            if (_update_mpe_fec) {
                base[4] = (base[4] & ~0x04) | uint8_t(_mpe_fec << 2);
            }
            if (_update_time_slicing) {
                base[4] = (base[4] & ~0x08) | uint8_t(_time_slicing << 3);
            }
        }
    }

    // Process all linkage descriptors
    for (size_t i = dlist.search(DID_DVB_LINKAGE); i < dlist.count(); i = dlist.search(DID_DVB_LINKAGE, i + 1)) {

        uint8_t* base = dlist[i].payload();
        size_t size = dlist[i].payloadSize();

        // If the linkage descriptor points to a removed TS, remove the descriptor
        if (size >= 2 && _remove_ts.count(GetUInt16 (base)) != 0) {
            dlist.removeByIndex(i);
            --i;
        }
    }

    // Process all service_list_descriptors
    if (_sld_oper == LCN_REMOVE) {
        // Completely remove all service_list_descriptors
        dlist.removeByTag(DID_DVB_SERVICE_LIST);
    }
    else {
        // Modify all service_list_descriptors
        for (size_t i = dlist.search(DID_DVB_SERVICE_LIST); i < dlist.count(); i = dlist.search(DID_DVB_SERVICE_LIST, i + 1)) {

            uint8_t* base = dlist[i].payload();
            size_t size = dlist[i].payloadSize();
            uint8_t* data = base;
            uint8_t* new_data = base;
            bool keep = true;

            while (size >= 3) {
                uint16_t id = GetUInt16(data);
                uint8_t type = data[2];
                switch (_sld_oper) {
                    case LCN_NONE: {
                        // No global modification, check other option
                        if (_remove_serv.count (id) == 0) {
                            PutUInt16(new_data, id);
                            new_data[2] = type;
                            new_data += 3;
                        }
                        break;
                    }
                    case LCN_REMOVE_ODD: {
                        // Remove one value every two values
                        if (keep) {
                            PutUInt16(new_data, id);
                            new_data[2] = type;
                            new_data += 3;
                        }
                        keep = !keep;
                        break;
                    }
                    default: {
                        // Should not get there
                        assert (false);
                    }
                }
                data += 3;
                size -= 3;
            }
            dlist[i].resizePayload(new_data - base);
        }
    }

    // Process all logical_channel_number_descriptors
    if (_lcn_oper == LCN_REMOVE) {
        // Completely remove all LCN descriptors
        dlist.removeByTag(DID_EACEM_LCN, PDS_EICTA);
    }
    else {
        // Modify all LCN descriptors
        for (size_t i = dlist.search(DID_EACEM_LCN, 0, PDS_EICTA);
             i < dlist.count();
             i = dlist.search(DID_EACEM_LCN, i + 1, PDS_EICTA)) {

            uint8_t* base = dlist[i].payload();
            size_t size = dlist[i].payloadSize();
            uint8_t* data = base;
            uint8_t* new_data = base;
            bool keep = true;
            uint16_t previous_lcn = 0;

            while (size >= 4) {
                uint16_t id = GetUInt16(data);
                uint16_t lcn = GetUInt16(data + 2);
                switch (_lcn_oper) {
                    case LCN_NONE: {
                        // No global modification, check other option
                        if (_remove_serv.count (id) == 0) {
                            PutUInt16(new_data, id);
                            PutUInt16(new_data + 2, lcn);
                            new_data += 4;
                        }
                        break;
                    }
                    case LCN_REMOVE_ODD: {
                        // Remove one value every two values
                        if (keep) {
                            PutUInt16(new_data, id);
                            PutUInt16(new_data + 2, lcn);
                            new_data += 4;
                        }
                        keep = !keep;
                        break;
                    }
                    case LCN_DUPLICATE_ODD: {
                        // Duplicate LCN values
                        PutUInt16(new_data, id);
                        if (keep) {
                            PutUInt16(new_data + 2, lcn);
                            previous_lcn = lcn;
                        }
                        else {
                            PutUInt16(new_data + 2, previous_lcn);
                        }
                        new_data += 4;
                        keep = !keep;
                        break;
                    }
                    default: {
                        // Should not get there
                        assert (false);
                    }
                }
                data += 4;
                size -= 4;
            }

            dlist[i].resizePayload(new_data - base);
        }
    }
}


//----------------------------------------------------------------------------
// Update the service list descriptors from collected services.
//----------------------------------------------------------------------------

void ts::NITPlugin::updateServiceList(NIT& nit)
{
    // Loop on all collected transport streams.
    for (const auto& it1 : _collected_sld) {
        const TransportStreamId& tsid(it1.first);
        const ServiceListDescriptor& sld(it1.second);

        // Only consider transport streams with collected services.
        if (!sld.entries.empty()) {

            // Get or create TS entry in the NIT.
            NIT::Transport& ts(nit.transports[tsid]);

            // Search an existing service list descriptor in this description.
            const size_t sld_index = ts.descs.search(DID_DVB_SERVICE_LIST);

            if (sld_index >= ts.descs.size()) {
                // No service list descriptor present, just add the collected one.
                ts.descs.add(duck, sld);
            }
            else {
                // There is an existing service list descriptor, merge the collected data.
                ServiceListDescriptor desc(duck, ts.descs[sld_index]);
                if (desc.isValid()) {
                    // Merge the descriptors.
                    for (const auto& it2 : sld.entries) {
                        desc.addService(it2.service_id, it2.service_type);
                    }
                }
                else {
                    // Invalid existing descriptor, use the collected one.
                    desc = sld;
                }
                // Remove all existing service list descriptors and add the merged one.
                ts.descs.removeByTag(DID_DVB_SERVICE_LIST);
                ts.descs.add(duck, desc);
            }
        }
    }
}

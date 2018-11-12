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
//  Transport stream processor shared library:
//  Various transformations on the NIT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsNetworkNameDescriptor.h"
#include "tsNIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITPlugin: public AbstractTablePlugin
    {
    public:
        // Implementation of plugin API
        NITPlugin(TSP*);
        virtual bool start() override;

    private:
        PID                _nit_pid;           // PID for the NIT (default: read PAT)
        UString            _new_netw_name;     // New network name
        bool               _set_netw_id;       // Change network id
        uint16_t           _new_netw_id;       // New network id
        bool               _use_nit_other;     // Use a NIT Other, not the NIT Actual
        uint16_t           _nit_other_id;      // Network id of the NIT Other to hack
        int                _lcn_oper;          // Operation on LCN descriptors
        int                _sld_oper;          // Operation on service_list_descriptors
        std::set<uint16_t> _remove_serv;       // Set of services to remove
        std::set<uint16_t> _remove_ts;         // Set of transport streams to remove
        std::vector<DID>   _removed_desc;      // Set of descriptor tags to remove
        PDS                _pds;               // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc; // Remove private desc without preceding PDS desc
        bool               _update_mpe_fec;    // In terrestrial delivery
        uint8_t            _mpe_fec;
        bool               _update_time_slicing;  // In terrestrial delivery
        uint8_t            _time_slicing;

        // Values for _lcn_oper and _sld_oper.
        enum {
            LCN_NONE          = 0,
            LCN_REMOVE        = 1,
            LCN_REMOVE_ODD    = 2,
            LCN_DUPLICATE_ODD = 3  // LCN only
        };

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert) override;

        // Process a list of descriptors according to the command line options.
        void processDescriptorList(DescriptorList&);

        // Inaccessible operations
        NITPlugin() = delete;
        NITPlugin(const NITPlugin&) = delete;
        NITPlugin& operator=(const NITPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(nit, ts::NITPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NITPlugin::NITPlugin(TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the NIT", u"[options]", u"NIT", PID_NIT),
    _nit_pid(PID_NIT),
    _new_netw_name(),
    _set_netw_id(false),
    _new_netw_id(0),
    _use_nit_other(false),
    _nit_other_id(0),
    _lcn_oper(0),
    _sld_oper(0),
    _remove_serv(),
    _remove_ts(),
    _removed_desc(),
    _pds(0),
    _cleanup_priv_desc(false),
    _update_mpe_fec(false),
    _mpe_fec(0),
    _update_time_slicing(false),
    _time_slicing(0)
{
    option(u"cleanup-private-descriptors", 0);
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

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
// Start method
//----------------------------------------------------------------------------

bool ts::NITPlugin::start()
{
    // Get option values
    _nit_pid = intValue<PID>(u"pid", PID_NULL);
    _lcn_oper = intValue<int>(u"lcn", LCN_NONE);
    _sld_oper = intValue<int>(u"sld", LCN_NONE);
    getIntValues(_remove_serv, u"remove-service");
    getIntValues(_remove_ts, u"remove-ts");
    getIntValues(_removed_desc, u"remove-descriptor");
    _pds = intValue<PDS>(u"pds");
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    _update_mpe_fec = present(u"mpe-fec");
    _mpe_fec = intValue<uint8_t>(u"mpe-fec") & 0x01;
    _update_time_slicing = present(u"time-slicing");
    _time_slicing = intValue<uint8_t>(u"time-slicing") & 0x01;
    _new_netw_name = value(u"network-name");
    _set_netw_id = present(u"network-id");
    _new_netw_id = intValue<uint16_t>(u"network-id");
    _use_nit_other = present(u"other") || present(u"nit-other");
    _nit_other_id = intValue<uint16_t>(u"other", intValue<uint16_t>(u"nit-other"));

    if (_lcn_oper != LCN_NONE && !_remove_serv.empty()) {
        tsp->error(u"--lcn and --remove-service are mutually exclusive");
        return false;
    }
    if (_sld_oper != LCN_NONE && !_remove_serv.empty()) {
        tsp->error(u"--sld and --remove-service are mutually exclusive");
        return false;
    }

    // Start superclass.
    return AbstractTablePlugin::start();
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

    nit.serialize(table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::NITPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert)
{
    // If not the NIT we are looking for, reinsert without modification.
    is_target =
        (!_use_nit_other && table.tableId() == TID_NIT_ACT) ||
        (_use_nit_other && table.tableId() == TID_NIT_OTH && table.tableIdExtension() == _nit_other_id);
    if (!is_target) {
        return;
    }

    // Process the NIT.
    NIT nit(table);
    if (!nit.isValid()) {
        tsp->warning(u"found invalid NIT");
        reinsert = false;
        return;
    }

    tsp->debug(u"got a NIT, version %d, network Id: %d (0x%X)", {nit.version, nit.network_id, nit.network_id});

    // Remove the specified transport streams
    bool found;
    do {
        found = false;
        for (NIT::TransportMap::iterator it = nit.transports.begin(); it != nit.transports.end(); ++it) {
            if (_remove_ts.count(it->first.transport_stream_id) != 0) {
                found = true;
                nit.transports.erase(it->first);
                break; // iterator is broken
            }
        }
    } while (found);

    // Update the network id.
    if (_set_netw_id) {
        nit.network_id = _new_netw_id;
    }

    // Update the network name.
    if (!_new_netw_name.empty()) {
        // Remove previous network_name_descriptor, if any.
        nit.descs.removeByTag(DID_NETWORK_NAME);
        // Add a new network_name_descriptor
        nit.descs.add(NetworkNameDescriptor(_new_netw_name));
    }

    // Process the global descriptor list
    processDescriptorList(nit.descs);

    // Process each TS descriptor list
    for (NIT::TransportMap::iterator it = nit.transports.begin(); it != nit.transports.end(); ++it) {
        processDescriptorList(it->second.descs);
    }

    // Reserialize modified NIT.
    nit.clearPreferredSections();
    nit.serialize(table);
}


//----------------------------------------------------------------------------
//  This method processes a NIT descriptor list
//----------------------------------------------------------------------------

void ts::NITPlugin::processDescriptorList(DescriptorList& dlist)
{
    // Process descriptor removal
    for (std::vector<DID>::const_iterator it = _removed_desc.begin(); it != _removed_desc.end(); ++it) {
        dlist.removeByTag(*it, _pds);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        dlist.removeInvalidPrivateDescriptors();
    }

    // Process all terrestrial_delivery_system_descriptors
    for (size_t i = dlist.search(DID_TERREST_DELIVERY); i < dlist.count(); i = dlist.search(DID_TERREST_DELIVERY, i + 1)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();

        if (size > 4) {
            if (_update_mpe_fec) {
                base[4] = (base[4] & ~0x04) | (_mpe_fec << 2);
            }
            if (_update_time_slicing) {
                base[4] = (base[4] & ~0x08) | (_time_slicing << 3);
            }
        }
    }

    // Process all linkage descriptors
    for (size_t i = dlist.search(DID_LINKAGE); i < dlist.count(); i = dlist.search(DID_LINKAGE, i + 1)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();

        // If the linkage descriptor points to a removed TS, remove the descriptor
        if (size >= 2 && _remove_ts.count(GetUInt16 (base)) != 0) {
            dlist.removeByIndex(i);
            --i;
        }
    }

    // Process all service_list_descriptors
    if (_sld_oper == LCN_REMOVE) {
        // Completely remove all service_list_descriptors
        dlist.removeByTag(DID_SERVICE_LIST);
    }
    else {
        // Modify all service_list_descriptors
        for (size_t i = dlist.search(DID_SERVICE_LIST); i < dlist.count(); i = dlist.search(DID_SERVICE_LIST, i + 1)) {

            uint8_t* base = dlist[i]->payload();
            size_t size = dlist[i]->payloadSize();
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
            dlist[i]->resizePayload(new_data - base);
        }
    }

    // Process all logical_channel_number_descriptors
    if (_lcn_oper == LCN_REMOVE) {
        // Completely remove all LCN descriptors
        dlist.removeByTag(DID_LOGICAL_CHANNEL_NUM, PDS_EICTA);
    }
    else {
        // Modify all LCN descriptors
        for (size_t i = dlist.search(DID_LOGICAL_CHANNEL_NUM, 0, PDS_EICTA);
             i < dlist.count();
             i = dlist.search(DID_LOGICAL_CHANNEL_NUM, i + 1, PDS_EICTA)) {

            uint8_t* base = dlist[i]->payload();
            size_t size = dlist[i]->payloadSize();
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

            dlist[i]->resizePayload(new_data - base);
        }
    }
}

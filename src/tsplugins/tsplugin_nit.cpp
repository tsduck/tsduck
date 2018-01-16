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

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsPAT.h"
#include "tsNIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        NITPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool               _abort;             // Error (service not found, etc)
        PID                _nit_pid;           // PID for the NIT (default: read PAT)
        bool               _incr_version;      // Increment table version
        bool               _set_version;       // Set a new table version
        uint8_t            _new_version;       // New table version
        int                _lcn_oper;          // Operation on LCN descriptors
        int                _sld_oper;          // Operation on service_list_descriptors
        std::set<uint16_t> _remove_serv;       // Set of services to remove
        std::set<uint16_t> _remove_ts;         // Set of transport streams to remove
        std::vector<DID>   _removed_desc;      // Set of descriptor tags to remove
        SectionDemux       _demux;             // Section demux
        CyclingPacketizer  _pzer;              // Packetizer for modified NIT
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

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        void processNIT(NIT&);
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
    ProcessorPlugin(tsp_, u"Perform various transformations on the NIT Actual.", u"[options]"),
    _abort(false),
    _nit_pid(PID_NIT),
    _incr_version(false),
    _set_version(false),
    _new_version(0),
    _lcn_oper(0),
    _sld_oper(0),
    _remove_serv(),
    _remove_ts(),
    _removed_desc(),
    _demux(this),
    _pzer(),
    _pds(0),
    _cleanup_priv_desc(false),
    _update_mpe_fec(false),
    _mpe_fec(0),
    _update_time_slicing(false),
    _time_slicing(0)
{
    option(u"cleanup-private-descriptors", 0);
    option(u"increment-version", 'i');
    option(u"lcn",               'l', INTEGER, 0, 1, 1, 3);
    option(u"mpe-fec",            0,  INTEGER, 0, 1, 0, 1);
    option(u"new-version",       'v', INTEGER, 0, 1, 0, 31);
    option(u"pds",                0,  UINT32);
    option(u"pid",               'p', PIDVAL);
    option(u"remove-descriptor",  0,  UINT8,   0, UNLIMITED_COUNT);
    option(u"remove-service",    'r', UINT16,  0, UNLIMITED_COUNT);
    option(u"remove-ts",          0,  UINT16,  0, UNLIMITED_COUNT);
    option(u"sld",               's', INTEGER, 0, 1, 1, 2);
    option(u"time-slicing",       0,  INTEGER, 0, 1, 0, 1);

    setHelp(u"Options:\n"
            u"\n"
            u"  --cleanup-private-descriptors\n"
            u"      Remove all private descriptors without preceding private_data_specifier\n"
            u"      descriptor.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --increment-version\n"
            u"      Increment the version number of the NIT.\n"
            u"\n"
            u"  -l value\n"
            u"  --lcn value\n"
            u"      Specify which operation to perform on logical_channel_number (LCN)\n"
            u"      descriptors. The value is a positive integer:\n"
            u"        1: Remove all LCN descriptors.\n"
            u"        2: Remove one entry every two entries in each LCN descriptor.\n"
            u"        3: Duplicate one entry every two entries in each LCN descriptor.\n"
            u"\n"
            u"  --mpe-fec value\n"
            u"      Set the \"MPE-FEC indicator\" in the terrestrial delivery system\n"
            u"      descriptors to the specified value (0 or 1).\n"
            u"\n"
            u"  -v value\n"
            u"  --new-version value\n"
            u"      Specify a new value for the version of the NIT.\n"
            u"\n"
            u"  --pds value\n"
            u"      With option --remove-descriptor, specify the private data specifier\n"
            u"      which applies to the descriptor tag values above 0x80.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specify the PID on which the NIT is expected. By default, the PAT\n"
            u"      is analyzed to get the PID of the NIT. DVB-compliant networks should\n"
            u"      use PID 16 (0x0010) for the NIT and signal it in the PAT.\n"
            u"\n"
            u"  --remove-descriptor value\n"
            u"      Remove from the NIT all descriptors with the specified tag. Several\n"
            u"      --remove-descriptor options may be specified to remove several types of\n"
            u"      descriptors. See also option --pds.\n"
            u"\n"
            u"  -r value\n"
            u"  --remove-service value\n"
            u"      Remove the specified service_id from the following descriptors:\n"
            u"      service_list_descriptor, logical_channel_number_descriptor.\n"
            u"      Several --remove-service options may be specified to remove several\n"
            u"      services.\n"
            u"\n"
            u"  --remove-ts value\n"
            u"      Remove the specified ts_id from the NIT. Several --remove-ts options\n"
            u"      may be specified to remove several TS.\n"
            u"\n"
            u"  -s value\n"
            u"  --sld value\n"
            u"      Specify which operation to perform on service_list_descriptors.\n"
            u"      The value is a positive integer:\n"
            u"        1: Remove all service_list_descriptors.\n"
            u"        2: Remove one entry every two entries in each descriptor.\n"
            u"\n"
            u"  --time-slicing value\n"
            u"      Set the \"time slicing indicator\" in the terrestrial delivery system\n"
            u"      descriptors to the specified value (0 or 1).\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
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
    _incr_version = present(u"increment-version");
    _set_version = present(u"new-version");
    _new_version = intValue<uint8_t>(u"new-version", 0);

    if (_lcn_oper != LCN_NONE && !_remove_serv.empty()) {
        tsp->error(u"--lcn and --remove-service are mutually exclusive");
        return false;
    }
    if (_sld_oper != LCN_NONE && !_remove_serv.empty()) {
        tsp->error(u"--sld and --remove-service are mutually exclusive");
        return false;
    }

    // Initialize the demux and packetizer
    _demux.reset();
    _pzer.reset();
    _pzer.setPID (_nit_pid);
    if (_nit_pid != PID_NULL) {
        // NIT PID is specified on the command line
        _demux.addPID (_nit_pid);
    }
    else {
        // Get the PAT to determine NIT PID
        _demux.addPID (PID_PAT);
    }

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::NITPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat (table);
                if (pat.isValid()) {
                    if ((_nit_pid = pat.nit_pid) == PID_NULL) {
                        _nit_pid = PID_NIT;
                        tsp->warning(u"NIT PID unspecified in PAT, using DVB default: %d (0x%X)", {_nit_pid, _nit_pid});
                    }
                    else {
                        tsp->verbose(u"NIT PID is %d (0x%X) in PAT", {_nit_pid, _nit_pid});
                    }
                    // No longer filter the PAT
                    _demux.removePID(PID_PAT);
                    // Now filter the NIT
                    _demux.addPID(_nit_pid);
                    _pzer.setPID(_nit_pid);
                }
            }
            break;
        }

        case TID_NIT_OTH: {
            if (table.sourcePID() == _nit_pid) {
                // NIT Other are passed unmodified
                _pzer.removeSections(TID_NIT_OTH, table.tableIdExtension());
                _pzer.addTable(table);
            }
            break;
        }

        case TID_NIT_ACT: {
            if (table.sourcePID() == _nit_pid) {
                // Modify NIT Actual
                NIT nit(table);
                if (nit.isValid()) {
                    // Transform NIT Actual
                    _pzer.removeSections(TID_NIT_ACT, nit.network_id);
                    processNIT(nit);
                    _pzer.addTable(nit);
                }
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a NIT
//----------------------------------------------------------------------------

void ts::NITPlugin::processNIT (NIT& nit)
{
    tsp->debug(u"got a NIT, version %d, network Id: %d (0x%X)", {nit.version, nit.network_id, nit.network_id});

    // Update NIT version
    if (_incr_version) {
        nit.version = (nit.version + 1) & 0x1F;
    }
    else if (_set_version) {
        nit.version = _new_version;
    }

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

    // Process the global descriptor list
    processDescriptorList(nit.descs);

    // Process each TS descriptor list
    for (NIT::TransportMap::iterator it = nit.transports.begin(); it != nit.transports.end(); ++it) {
        processDescriptorList(it->second);
    }
}


//----------------------------------------------------------------------------
//  This method processes a NIT descriptor list
//----------------------------------------------------------------------------

void ts::NITPlugin::processDescriptorList (DescriptorList& dlist)
{
    // Process descriptor removal
    for (std::vector<DID>::const_iterator it = _removed_desc.begin(); it != _removed_desc.end(); ++it) {
        dlist.removeByTag (*it, _pds);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        dlist.removeInvalidPrivateDescriptors();
    }

    // Process all terrestrial_delivery_system_descriptors
    for (size_t i = dlist.search (DID_TERREST_DELIVERY); i < dlist.count(); i = dlist.search (DID_TERREST_DELIVERY, i + 1)) {

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
    for (size_t i = dlist.search (DID_LINKAGE); i < dlist.count(); i = dlist.search (DID_LINKAGE, i + 1)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();

        // If the linkage descriptor points to a removed TS, remove the descriptor
        if (size >= 2 && _remove_ts.count (GetUInt16 (base)) != 0) {
            dlist.removeByIndex (i);
            --i;
        }
    }

    // Process all service_list_descriptors
    if (_sld_oper == LCN_REMOVE) {
        // Completely remove all service_list_descriptors
        dlist.removeByTag (DID_SERVICE_LIST);
    }
    else {
        // Modify all service_list_descriptors
        for (size_t i = dlist.search (DID_SERVICE_LIST); i < dlist.count(); i = dlist.search (DID_SERVICE_LIST, i + 1)) {

            uint8_t* base = dlist[i]->payload();
            size_t size = dlist[i]->payloadSize();
            uint8_t* data = base;
            uint8_t* new_data = base;
            bool keep = true;

            while (size >= 3) {
                uint16_t id = GetUInt16 (data);
                uint8_t type = data[2];
                switch (_sld_oper) {
                    case LCN_NONE: {
                        // No global modification, check other option
                        if (_remove_serv.count (id) == 0) {
                            PutUInt16 (new_data, id);
                            new_data[2] = type;
                            new_data += 3;
                        }
                        break;
                    }
                    case LCN_REMOVE_ODD: {
                        // Remove one value every two values
                        if (keep) {
                            PutUInt16 (new_data, id);
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
            dlist[i]->resizePayload (new_data - base);
        }
    }

    // Process all logical_channel_number_descriptors
    if (_lcn_oper == LCN_REMOVE) {
        // Completely remove all LCN descriptors
        dlist.removeByTag (DID_LOGICAL_CHANNEL_NUM, PDS_EICTA);
    }
    else {
        // Modify all LCN descriptors
        for (size_t i = dlist.search (DID_LOGICAL_CHANNEL_NUM, 0, PDS_EICTA);
             i < dlist.count();
             i = dlist.search (DID_LOGICAL_CHANNEL_NUM, i + 1, PDS_EICTA)) {

            uint8_t* base = dlist[i]->payload();
            size_t size = dlist[i]->payloadSize();
            uint8_t* data = base;
            uint8_t* new_data = base;
            bool keep = true;
            uint16_t previous_lcn = 0;

            while (size >= 4) {
                uint16_t id = GetUInt16 (data);
                uint16_t lcn = GetUInt16 (data + 2);
                switch (_lcn_oper) {
                    case LCN_NONE: {
                        // No global modification, check other option
                        if (_remove_serv.count (id) == 0) {
                            PutUInt16 (new_data, id);
                            PutUInt16 (new_data + 2, lcn);
                            new_data += 4;
                        }
                        break;
                    }
                    case LCN_REMOVE_ODD: {
                        // Remove one value every two values
                        if (keep) {
                            PutUInt16 (new_data, id);
                            PutUInt16 (new_data + 2, lcn);
                            new_data += 4;
                        }
                        keep = !keep;
                        break;
                    }
                    case LCN_DUPLICATE_ODD: {
                        // Duplicate LCN values
                        PutUInt16 (new_data, id);
                        if (keep) {
                            PutUInt16 (new_data + 2, lcn);
                            previous_lcn = lcn;
                        }
                        else {
                            PutUInt16 (new_data + 2, previous_lcn);
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

            dlist[i]->resizePayload (new_data - base);
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::NITPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // As long as NIT PID is unknown, nullify packets to avoid transmission of the unmodified NIT
    if (_nit_pid == PID_NULL) {
        return TSP_NULL;
    }

    // Replace packets using packetizer
    if (pkt.getPID() == _nit_pid) {
        _pzer.getNextPacket (pkt);
    }

    return TSP_OK;
}

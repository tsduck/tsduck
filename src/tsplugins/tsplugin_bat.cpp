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
//  Various transformations on the BAT.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsServiceDescriptor.h"
#include "tsService.h"
#include "tsBAT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BATPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        BATPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool               _abort;             // Error (service not found, etc)
        bool               _single_bat;        // Modify one single BAT only
        uint16_t           _bouquet_id;        // Bouquet id of the BAT to modify (if _single_bat)
        bool               _incr_version;      // Increment table version
        bool               _set_version;       // Set a new table version
        uint8_t            _new_version;       // New table version
        std::set<uint16_t> _remove_serv;       // Set of services to remove
        std::set<uint16_t> _remove_ts;         // Set of transport streams to remove
        std::vector<DID>   _removed_desc;      // Set of descriptor tags to remove
        PDS                _pds;               // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc; // Remove private desc without preceding PDS desc
        SectionDemux       _demux;             // Section demux
        CyclingPacketizer  _pzer;              // Packetizer for modified SDT/BAT

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&) override;
        void processBAT (BAT&);
        void processDescriptorList (DescriptorList&);

        // Inaccessible operations
        BATPlugin() = delete;
        BATPlugin(const BATPlugin&) = delete;
        BATPlugin& operator=(const BATPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(bat, ts::BATPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BATPlugin::BATPlugin (TSP* tsp_) :
   ProcessorPlugin(tsp_, u"Perform various transformations on the BAT", u"[options]"),
   _abort(false),
   _single_bat(false),
   _bouquet_id(0),
   _incr_version(false),
   _set_version(false),
   _new_version(0),
   _remove_serv(),
   _remove_ts(),
   _removed_desc(),
   _pds(0),
   _cleanup_priv_desc(false),
   _demux(this),
   _pzer()
{
    option(u"bouquet-id", 'b', UINT16);
    help(u"bouquet-id",
         u"Specify the bouquet id of the BAT to modify and leave other BAT's "
         u"unmodified. By default, all BAT's are modified.");

    option(u"cleanup-private-descriptors");
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

    option(u"increment-version", 'i');
    help(u"increment-version",
         u"Increment the version number of the BAT.");

    option(u"new-version", 'v', INTEGER, 0, 1, 0, 31);
    help(u"new-version",
         u"Specify a new value for the version of the BAT.");

    option(u"pds", 0, UINT32);
    help(u"pds",
         u"With option --remove-descriptor, specify the private data specifier "
         u"which applies to the descriptor tag values above 0x80.");

    option(u"remove-descriptor", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"remove-descriptor",
         u"Remove from the BAT all descriptors with the specified tag. Several "
         u"--remove-descriptor options may be specified to remove several types of "
         u"descriptors. See also option --pds.");

    option(u"remove-service", 'r', UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-service",
         u"Remove the specified service_id from the following descriptors: "
         u"service_list_descriptor, logical_channel_number_descriptor. "
         u"Several --remove-service options may be specified to remove several services.");

    option(u"remove-ts", 0, UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-ts",
         u"Remove the specified ts_id from the BAT. Several --remove-ts options "
         u"may be specified to remove several TS.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::BATPlugin::start()
{
    // Get option values
    _single_bat = present(u"bouquet-id");
    _bouquet_id = intValue<uint16_t>(u"bouquet-id", 0);
    _pds = intValue<PDS>(u"pds", 0);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    _incr_version = present(u"increment-version");
    _set_version = present(u"new-version");
    _new_version = intValue<uint8_t>(u"new-version", 0);
    getIntValues(_remove_serv, u"remove-service");
    getIntValues(_remove_ts, u"remove-ts");
    getIntValues(_removed_desc, u"remove-descriptor");

    // Initialize the demux and packetizer
    _demux.reset();
    _demux.addPID (PID_BAT);
    _pzer.reset();
    _pzer.setPID (PID_BAT);

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::BATPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                BAT bat (table);
                if (bat.isValid()) {
                    _pzer.removeSections (TID_BAT, table.tableIdExtension());
                    if (!_single_bat || table.tableIdExtension() == _bouquet_id) {
                        processBAT (bat);
                    }
                    _pzer.addTable (bat);
                }
            }
            break;
        }

        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_BAT) {
                // Do not modify SDT (same PID as BAT)
                _pzer.removeSections (table.tableId(), table.tableIdExtension());
                _pzer.addTable (table);
            }
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a BAT
//----------------------------------------------------------------------------

void ts::BATPlugin::processBAT(BAT& bat)
{
    tsp->debug(u"got a BAT, version %d, bouquet id: %d (0x%X)", {bat.version, bat.bouquet_id, bat.bouquet_id});

    // Update BAT version
    if (_incr_version) {
        bat.version = (bat.version + 1) & 0x1F;
    }
    else if (_set_version) {
        bat.version = _new_version;
    }

    // Remove the specified transport streams
    bool found;
    do {
        found = false;
        for (BAT::TransportMap::iterator it = bat.transports.begin(); it != bat.transports.end(); ++it) {
            if (_remove_ts.count (it->first.transport_stream_id) != 0) {
                found = true;
                bat.transports.erase (it->first);
                break; // iterator is broken
            }
        }
    } while (found);

    // Process the global descriptor list
    processDescriptorList(bat.descs);

    // Process each TS descriptor list
    for (BAT::TransportMap::iterator it = bat.transports.begin(); it != bat.transports.end(); ++it) {
        processDescriptorList(it->second.descs);
    }
}


//----------------------------------------------------------------------------
//  This method processes a BAT descriptor list
//----------------------------------------------------------------------------

void ts::BATPlugin::processDescriptorList(DescriptorList& dlist)
{
    // Process descriptor removal
    for (std::vector<DID>::const_iterator it = _removed_desc.begin(); it != _removed_desc.end(); ++it) {
        dlist.removeByTag (*it, _pds);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        dlist.removeInvalidPrivateDescriptors();
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
    for (size_t i = dlist.search (DID_SERVICE_LIST);
         i < dlist.count();
         i = dlist.search (DID_SERVICE_LIST, i + 1)) {

        uint8_t* const base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();
        uint8_t* data = base;
        uint8_t* new_data = base;
        while (size >= 3) {
            uint16_t id = GetUInt16 (data);
            uint8_t type = data[2];
            if (_remove_serv.count (id) == 0) {
                PutUInt16 (new_data, id);
                new_data[2] = type;
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
            uint16_t id = GetUInt16 (data);
            uint16_t lcn = GetUInt16 (data + 2);
            if (_remove_serv.count (id) == 0) {
                PutUInt16 (new_data, id);
                PutUInt16 (new_data + 2, lcn);
                new_data += 4;
            }
            data += 4;
            size -= 4;
        }
        dlist[i]->resizePayload (new_data - base);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BATPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Replace packets using packetizer
    if (pkt.getPID() == PID_BAT) {
        _pzer.getNextPacket (pkt);
    }

    return TSP_OK;
}

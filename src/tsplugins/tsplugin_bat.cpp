//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsServiceDescriptor.h"
#include "tsService.h"
#include "tsBAT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BATPlugin: public AbstractTablePlugin
    {
        TS_NOBUILD_NOCOPY(BATPlugin);
    public:
        // Implementation of plugin API
        BATPlugin(TSP*);
        virtual bool getOptions() override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert) override;

    private:
        // Command line options:
        bool               _single_bat;        // Modify one single BAT only
        uint16_t           _bouquet_id;        // Bouquet id of the BAT to modify (if _single_bat)
        std::set<uint16_t> _remove_serv;       // Set of services to remove
        std::set<uint16_t> _remove_ts;         // Set of transport streams to remove
        std::vector<DID>   _removed_desc;      // Set of descriptor tags to remove
        PDS                _pds;               // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc; // Remove private desc without preceding PDS desc

        // Process a list of descriptors according to the command line options.
        void processDescriptorList(DescriptorList&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"bat", ts::BATPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BATPlugin::BATPlugin(TSP* tsp_) :
   AbstractTablePlugin(tsp_, u"Perform various transformations on the BAT", u"[options]", u"BAT", PID_BAT),
   _single_bat(false),
   _bouquet_id(0),
   _remove_serv(),
   _remove_ts(),
   _removed_desc(),
   _pds(0),
   _cleanup_priv_desc(false)
{
    option(u"bouquet-id", 'b', UINT16);
    help(u"bouquet-id",
         u"Specify the bouquet id of the BAT to modify and leave other BAT's "
         u"unmodified. By default, all BAT's are modified.");

    option(u"cleanup-private-descriptors");
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

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
// Get options method
//----------------------------------------------------------------------------

bool ts::BATPlugin::getOptions()
{
    // Get option values
    _single_bat = present(u"bouquet-id");
    _bouquet_id = intValue<uint16_t>(u"bouquet-id", 0);
    _pds = intValue<PDS>(u"pds", 0);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    getIntValues(_remove_serv, u"remove-service");
    getIntValues(_remove_ts, u"remove-ts");
    getIntValues(_removed_desc, u"remove-descriptor");

    // Start superclass.
    return AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::BATPlugin::createNewTable(BinaryTable& table)
{
    BAT bat;

    // If we must modify one specific BAT, this is the one we need to create.
    if (_single_bat) {
        bat.bouquet_id = _bouquet_id;
    }

    bat.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::BATPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert)
{
    // If not a BAT (typically an SDT) or not the BAT we are looking for, reinsert without modification.
    if (table.tableId() != TID_BAT || (_single_bat && table.tableIdExtension() != _bouquet_id)) {
        is_target = false;
        return;
    }

    // Process the BAT.
    BAT bat(duck, table);
    if (!bat.isValid()) {
        tsp->warning(u"found invalid BAT");
        reinsert = false;
        return;
    }

    tsp->debug(u"got a BAT, version %d, bouquet id: %d (0x%X)", {bat.version, bat.bouquet_id, bat.bouquet_id});

    // Remove the specified transport streams
    bool found;
    do {
        found = false;
        for (BAT::TransportMap::iterator it = bat.transports.begin(); it != bat.transports.end(); ++it) {
            if (_remove_ts.count(it->first.transport_stream_id) != 0) {
                found = true;
                bat.transports.erase(it->first);
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

    // Reserialize modified BAT.
    bat.clearPreferredSections();
    bat.serialize(duck, table);
}


//----------------------------------------------------------------------------
//  This method processes a BAT descriptor list
//----------------------------------------------------------------------------

void ts::BATPlugin::processDescriptorList(DescriptorList& dlist)
{
    // Process descriptor removal
    for (std::vector<DID>::const_iterator it = _removed_desc.begin(); it != _removed_desc.end(); ++it) {
        dlist.removeByTag(*it, _pds);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        dlist.removeInvalidPrivateDescriptors();
    }

    // Process all linkage descriptors
    for (size_t i = dlist.search(DID_LINKAGE); i < dlist.count(); i = dlist.search (DID_LINKAGE, i + 1)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();

        // If the linkage descriptor points to a removed TS, remove the descriptor
        if (size >= 2 && _remove_ts.count(GetUInt16 (base)) != 0) {
            dlist.removeByIndex (i);
            --i;
        }
    }

    // Process all service_list_descriptors
    for (size_t i = dlist.search(DID_SERVICE_LIST);
         i < dlist.count();
         i = dlist.search(DID_SERVICE_LIST, i + 1)) {

        uint8_t* const base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();
        uint8_t* data = base;
        uint8_t* new_data = base;
        while (size >= 3) {
            uint16_t id = GetUInt16(data);
            uint8_t type = data[2];
            if (_remove_serv.count(id) == 0) {
                PutUInt16(new_data, id);
                new_data[2] = type;
                new_data += 3;
            }
            data += 3;
            size -= 3;
        }
        dlist[i]->resizePayload(new_data - base);
    }

    // Process all logical_channel_number_descriptors
    for (size_t i = dlist.search(DID_LOGICAL_CHANNEL_NUM, 0, PDS_EICTA);
         i < dlist.count();
         i = dlist.search(DID_LOGICAL_CHANNEL_NUM, i + 1, PDS_EICTA)) {

        uint8_t* base = dlist[i]->payload();
        size_t size = dlist[i]->payloadSize();
        uint8_t* data = base;
        uint8_t* new_data = base;
        while (size >= 4) {
            uint16_t id = GetUInt16(data);
            uint16_t lcn = GetUInt16(data + 2);
            if (_remove_serv.count(id) == 0) {
                PutUInt16(new_data, id);
                PutUInt16(new_data + 2, lcn);
                new_data += 4;
            }
            data += 4;
            size -= 4;
        }
        dlist[i]->resizePayload(new_data - base);
    }
}

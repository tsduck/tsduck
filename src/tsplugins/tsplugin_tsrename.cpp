//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Rename the transport stream
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsNames.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsNIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TSRenamePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        TSRenamePlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        bool              _abort;          // Error (service not found, etc)
        bool              _ready;          // Ready to pass packets
        PID               _nit_pid;        // PID for the NIT
        uint16_t          _old_ts_id;      // Old transport stream id
        bool              _set_ts_id;      // Modify transport stream id
        uint16_t          _new_ts_id;      // New transport stream id
        bool              _set_onet_id;    // Update original network id
        uint16_t          _new_onet_id;    // New original network id
        bool              _ignore_bat;     // Do not modify the BAT
        bool              _ignore_nit;     // Do not modify the NIT
        bool              _add_bat;        // Add a new TS entry in the BAT instead of replacing
        bool              _add_nit;        // Add a new TS entry in the NIT instead of replacing
        SectionDemux      _demux;          // Section demux
        CyclingPacketizer _pzer_pat;       // Packetizer for modified PAT
        CyclingPacketizer _pzer_sdt_bat;   // Packetizer for modified SDT/BAT
        CyclingPacketizer _pzer_nit;       // Packetizer for modified NIT

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Process specific tables and descriptors
        void processPAT (PAT&);
        void processSDT (SDT&);
        void processNITBAT (AbstractTransportListTable&, bool);

        // Inaccessible operations
        TSRenamePlugin() = delete;
        TSRenamePlugin(const TSRenamePlugin&) = delete;
        TSRenamePlugin& operator=(const TSRenamePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::TSRenamePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TSRenamePlugin::TSRenamePlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Rename a transport stream.", "[options]"),
    _abort(false),
    _ready(false),
    _nit_pid(PID_NIT),
    _old_ts_id(0),
    _set_ts_id(false),
    _new_ts_id(0),
    _set_onet_id(false),
    _new_onet_id(0),
    _ignore_bat(false),
    _ignore_nit(false),
    _add_bat(false),
    _add_nit(false),
    _demux(this),
    _pzer_pat(PID_PAT, CyclingPacketizer::ALWAYS),
    _pzer_sdt_bat(PID_SDT, CyclingPacketizer::ALWAYS),
    _pzer_nit(PID_NIT, CyclingPacketizer::ALWAYS)
{
    option(u"add",                 'a');
    option(u"add-bat",              0);
    option(u"add-nit",              0);
    option(u"ignore-bat",           0);
    option(u"ignore-nit",           0);
    option(u"original-network-id", 'o',  UINT16);
    option(u"ts-id",               't',  UINT16);

    setHelp(u"Options:\n"
             u"\n"
             u"  -a\n"
             u"  --add\n"
             u"      Equivalent to --add-bat --add-nit.\n"
             u"\n"
             u"  --add-bat\n"
             u"      Add a new entry for the renamed TS in the BAT and keep the previous\n"
             u"      entry. By default, the TS entry is renamed.\n"
             u"\n"
             u"  --add-nit\n"
             u"      Add a new entry for the renamed TS in the NIT and keep the previous\n"
             u"      entry. By default, the TS entry is renamed.\n"
             u"\n"
             u"  --help\n"
             u"      Display this help text.\n"
             u"\n"
             u"  --ignore-bat\n"
             u"      Do not modify the BAT.\n"
             u"\n"
             u"  --ignore-nit\n"
             u"      Do not modify the NIT.\n"
             u"\n"
             u"  -o value\n"
             u"  --original-network-id value\n"
             u"      Modify the original network id. By default, it is unchanged.\n"
             u"\n"
             u"  -t value\n"
             u"  --ts-id value\n"
             u"      Modify the transport stream id. By default, it is unchanged.\n"
             u"\n"
             u"  --version\n"
             u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TSRenamePlugin::start()
{
    // Get option values
    _add_bat = present(u"add") || present(u"add-bat");
    _add_nit = present(u"add") || present(u"add-nit");
    _ignore_bat = present(u"ignore-bat");
    _ignore_nit = present(u"ignore-nit");
    _set_onet_id = present(u"original-network-id");
    _new_onet_id = intValue<uint16_t> ("original-network-id", 0);
    _set_ts_id = present(u"ts-id");
    _new_ts_id = intValue<uint16_t> ("ts-id", 0);

    // Initialize the demux
    _demux.reset();
    _demux.addPID (PID_PAT);

    // Reset other states
    _abort = false;
    _ready = false;
    _old_ts_id = 0;
    _pzer_pat.reset();
    _pzer_sdt_bat.reset();
    _pzer_nit.reset();

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::TSRenamePlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    if (tsp->debug()) {
        std::string name(names::TID(table.tableId()).toUTF8());
        tsp->debug("Got %s v%d, PID %d (0x%04X), TIDext %d (0x%04X)",
                   name.c_str(), int(table.version()),
                   int(table.sourcePID()), int(table.sourcePID()),
                   int(table.tableIdExtension()), int(table.tableIdExtension()));
    }

    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat (table);
                if (pat.isValid()) {
                    processPAT (pat);
                }
            }
            break;
        }

        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt (table);
                if (sdt.isValid()) {
                    processSDT (sdt);
                }
            }
            break;
        }

        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                // SDT Other are passed unmodified
                _pzer_sdt_bat.removeSections (TID_SDT_OTH, table.tableIdExtension());
                _pzer_sdt_bat.addTable (table);
            }
            break;
        }

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                if (_ignore_bat) {
                    // Do not modify BAT
                    _pzer_sdt_bat.removeSections (TID_BAT, table.tableIdExtension());
                    _pzer_sdt_bat.addTable (table);
                }
                else {
                    // Modify BAT
                    BAT bat (table);
                    if (bat.isValid()) {
                        processNITBAT (bat, _add_bat);
                        _pzer_sdt_bat.removeSections (TID_BAT, bat.bouquet_id);
                        _pzer_sdt_bat.addTable (bat);
                    }
                }
            }
            break;
        }

        case TID_NIT_ACT: {
            if (!_ignore_nit) {
                // Modify NIT Actual
                NIT nit (table);
                if (nit.isValid()) {
                    processNITBAT (nit, _add_nit);
                    _pzer_nit.removeSections (TID_NIT_ACT, nit.network_id);
                    _pzer_nit.addTable (nit);
                }
            }
            break;
        }

        case TID_NIT_OTH: {
            if (!_ignore_nit) {
                // NIT Other are passed unmodified
                _pzer_nit.removeSections (TID_NIT_OTH, table.tableIdExtension());
                _pzer_nit.addTable (table);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::TSRenamePlugin::processPAT (PAT& pat)
{
    // Save the NIT PID
    _nit_pid = pat.nit_pid != PID_NULL ? pat.nit_pid : uint16_t (PID_NIT);
    _pzer_nit.setPID (_nit_pid);

    // Rename the TS
    _old_ts_id = pat.ts_id;
    if (_set_ts_id) {
        pat.ts_id = _new_ts_id;
    }

    // Replace the PAT.in the PID
    _pzer_pat.removeSections (TID_PAT);
    _pzer_pat.addTable (pat);

    // We are now ready to process the TS
    _demux.addPID (PID_SDT);
    if (!_ignore_nit) {
        _demux.addPID (_nit_pid);
    }
    _ready = true;
}


//----------------------------------------------------------------------------
//  This method processes a Service Description Table (SDT).
//----------------------------------------------------------------------------

void ts::TSRenamePlugin::processSDT (SDT& sdt)
{
    // Rename the TS
    if (_set_ts_id) {
        sdt.ts_id = _new_ts_id;
    }
    if (_set_onet_id) {
        sdt.onetw_id = _new_onet_id;
    }

    // Replace the SDT.in the PID
    _pzer_sdt_bat.removeSections (TID_SDT_ACT, sdt.ts_id);
    _pzer_sdt_bat.addTable (sdt);
}


//----------------------------------------------------------------------------
//  This method processes a NIT or a BAT
//----------------------------------------------------------------------------

void ts::TSRenamePlugin::processNITBAT (AbstractTransportListTable& table, bool add_entry)
{
    // Locate the transport stream, ignore original network id
    for (AbstractTransportListTable::TransportMap::iterator it = table.transports.begin(); it != table.transports.end(); ++it) {
        if (it->first.transport_stream_id == _old_ts_id) {

            const TransportStreamId new_tsid (_set_ts_id ? _new_ts_id : it->first.transport_stream_id,
                                              _set_onet_id ? _new_onet_id : it->first.original_network_id);

            if (new_tsid != it->first) {
                // Add a new TS entry
                table.transports [new_tsid] = it->second;
                if (!add_entry) {
                    table.transports.erase (it->first);
                }
            }

            return;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TSRenamePlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // As long as the original TS id is unknown, nullify packets
    if (!_ready) {
        return TSP_NULL;
    }

    // Replace packets using packetizers
    if (pid == PID_PAT) {
        _pzer_pat.getNextPacket (pkt);
    }
    else if (pid == PID_SDT) {
        _pzer_sdt_bat.getNextPacket (pkt);
    }
    else if (!_ignore_nit && pid == _nit_pid) {
        _pzer_nit.getNextPacket (pkt);
    }

    return TSP_OK;
}

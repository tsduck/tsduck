//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Extract DSM-CC Object Carousel content.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsZlib.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DSMCCPlugin: public ProcessorPlugin, private TableHandlerInterface {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);

    public:
        // Implementation of plugin API
        virtual bool   start() override;
        virtual bool   getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using ModulesMap = std::map<uint16_t, ByteBlock>;

        bool         _abort = false;               // Error (not found, etc).
        PID          _pid = PID_NULL;              // Carousel PID.
        bool         _dsi_found = false;           // DSI found flag
        bool         _dii_found = false;           // DII found flag
        bool         _all_modules_found = false;   // All modules found flag
        uint16_t     _number_of_modules = 0x0000;  // Number of modules
        ModulesMap   _modules {};                  // Map of modules data
        SectionDemux _demux {duck, this};          // Section filter

        bool _dump_ready = false;

        std::ostream* _out = &std::cout;

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processUNM(const DSMCCUserToNetworkMessage&);
        void processDDM(const DSMCCDownloadDataMessage&);
    };
}  // namespace ts

TS_REGISTER_PROCESSOR_PLUGIN(u"dsmcc", ts::DSMCCPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCPlugin::DSMCCPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract DSM-CC content", u"[options]")
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specifies the PID carrying DSM-CC Object Carousel.");
}

//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::getOptions()
{
    getIntValue(_pid, u"pid");
    verbose(u"get options pid: %n", _pid);
    return true;
}
//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::start()
{
    verbose(u"start");
    // Get command line arguments.
    duck.loadArgs(*this);
    getIntValue(_pid, u"pid", PID_NULL);

    // Reinitialize the plugin state.
    _abort = false;
    _dsi_found = false;
    _dii_found = false;
    _all_modules_found = false;
    _dump_ready = false;
    _number_of_modules = 0x0000;
    _modules.clear();
    _demux.reset();

    if (_pid != PID_NULL) {
        verbose(u"_demux.addPID: %n", _pid);
        _demux.addPID(_pid);
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    verbose(u"handleTable TID: %n", table.tableId());

    switch (table.tableId()) {

        case TID_DSMCC_UNM: {
            DSMCCUserToNetworkMessage unm(duck, table);
            if (unm.isValid()) {
                processUNM(unm);
            }
            break;
        }

        case TID_DSMCC_DDM: {
            DSMCCDownloadDataMessage ddm(duck, table);
            if (ddm.isValid()) {
                processDDM(ddm);
            }
            break;
        }

        default: {
            break;
        }
    }

    if (_all_modules_found) {
        verbose(u"Removing PID: %n", _pid);
        _demux.removePID(_pid);
    }
}


//----------------------------------------------------------------------------
//  This method processes a DSM-CC User-to-Network Message.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::processUNM(const DSMCCUserToNetworkMessage& unm)
{
    verbose(u"processUNM");

    if (unm.header.message_id == 0x1006) {
        _dsi_found = true;
        verbose(u"DSI found");
    }
    else if (unm.header.message_id == 0x1002) {
        _dii_found = true;
        verbose(u"DII found");
        verbose(u"number of modules: %n", unm.modules.size());
        _number_of_modules = unm.modules.size();
    }
}

//----------------------------------------------------------------------------
//  This method processes a DSM-CC Download Data Message.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::processDDM(const DSMCCDownloadDataMessage& ddm)
{
    verbose(u"processDDM");

    _modules[ddm.module_id] = ddm.block_data;

    verbose(u"Number of modules: %n", _modules.size());

    if (_dsi_found && _dii_found) {
        verbose(u"dsi and dii found!!!");
        verbose(u"modules captured: %d", (uint16_t)_number_of_modules);
        if (_number_of_modules == (uint16_t)_modules.size()) {
            _all_modules_found = true;
            verbose(u"ALL MODULES FOUND!");
            for (auto& module : _modules) {

                /*if (module.first == 0x0001 && !_dump_ready) {*/
                ByteBlock compressed_module = module.second;
                ByteBlock uncompressed_module {};
                Zlib::Decompress(uncompressed_module, compressed_module);

                uint32_t _hexa_flags = 0;
                _hexa_flags = UString::HEXA | UString::ASCII;

                if (true) {
                    *_out << UString::Format(u"COMPRESSED MODULE ID: %n", module.first)
                          << std::endl
                          << UString::Dump(compressed_module, _hexa_flags, 16)
                          << std::endl;


                    *_out << std::endl
                          << UString::Format(u"UNCOMPRESSED MODULE ID: %n", module.first)
                          << std::endl
                          << UString::Dump(uncompressed_module, _hexa_flags, 16)
                          << std::endl;
                }
                /*_dump_ready = true;*/
                /*}*/
            }
        }
    }
}

//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return _abort ? TSP_END : TSP_OK;
}

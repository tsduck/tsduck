//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to scan the NIT to get a list of tuning informations for all TS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsChannelFile.h"
#include "tsPAT.h"
#include "tsNIT.h"

namespace ts {
    //!
    //! Plugin to scan the NIT to get a list of tuning informations for all transport streams.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL NITScanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(NITScanPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        fs::path      _output_name {};                // Output file name
        std::ofstream _output_stream {};              // Output file stream
        std::ostream* _output = nullptr;              // Actual output stream
        UString       _comment_prefix {};             // Prefix for comment lines
        UString       _variable_prefix {};            // Prefix for environment variable names
        bool          _use_comment = false;           // Add comment line
        bool          _use_variable = false;          // Environment variable format
        bool          _terminate = false;             // Terminate after one NIT
        bool          _dvb_options = false;           // Output format: dvb plugin options
        bool          _all_nits = false;              // Also include all "NIT other"
        bool          _nit_other = false;             // Analyze one "NIT other"
        uint16_t      _network_id = 0;                // Network id of "NIT other" to analyze
        PID           _nit_pid = PID_NULL;            // PID for the NIT (default: read PAT)
        size_t        _nit_count = 0;                 // Number of analyzed NIT's
        SectionDemux  _demux {duck, this};            // Section demux
        ChannelFile   _channels {};                   // Channel database
        fs::path      _channel_file {};               // Name of channel configuration file.
        bool          _save_channel_file = false;     // Save a fresh new version of channel configuration file.
        bool          _update_channel_file = false;   // Update previous content of channel configuration file.
        bool          _default_channel_file = false;  // Use default channel configuration file.

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(const PAT&);
        void processNIT(const NIT&);
    };
}

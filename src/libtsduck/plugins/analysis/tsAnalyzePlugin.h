//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream analyzer plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSAnalyzerReport.h"
#include "tsTSSpeedMetrics.h"
#include "tsFileNameGenerator.h"

namespace ts {
    //!
    //! Transport stream analyzer plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL AnalyzePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(AnalyzePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        fs::path          _output_name {};
        cn::nanoseconds   _output_interval {};
        bool              _multiple_output = false;
        bool              _cumulative = false;
        TSAnalyzerArgs    _analyzer_options {this};

        // Working data:
        std::ofstream     _output_stream {};
        std::ostream*     _output = nullptr;
        TSSpeedMetrics    _metrics {};
        cn::nanoseconds   _next_report {};
        TSAnalyzerReport  _analyzer {duck};
        FileNameGenerator _name_gen {};

        bool openOutput();
        void closeOutput();
        bool produceReport();
    };
}

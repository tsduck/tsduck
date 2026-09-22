//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Tuner device input plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsjsonOutputArgs.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Tuner device input plugin.
    //! Was DVB only initially, any tuner now.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DVBInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DVBInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;
        virtual bool abortInput() override;

        // Larger stack size than default
        virtual size_t stackUsage() const override {return 512 * 1024;} // 512 kB

    private:
        Tuner            _tuner {duck};          // DVB tuner device.
        TunerArgs        _tuner_args {false};    // Command-line tuning arguments.
        json::OutputArgs _json_args {this};      // JSON status reporting.
        cn::seconds      _json_interval {};      // Interval between JSON status reports.
        BitRate          _previous_bitrate = 0;  // Previous value from getBitrate().
        Time             _next_json_report {};   // UTC time of next JSON report.

        static constexpr cn::seconds DEFAULT_JSON_INTERVAL = cn::seconds(60);

        // Produce a JSON status report if necessary.
        void jsonReport();

        // Store the tuning parameters in a global repository (may be used by other plugins).
        void storeTunerArgs();
    };
}

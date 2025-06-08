//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::DektecOutputPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsBitrateDifferenceDVBT.h"

namespace ts {
    //!
    //! Dektec output plugin for @c tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDEKTECDLL DektecOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DektecOutputPlugin);
    public:
        //! Destructor.
        virtual ~DektecOutputPlugin() override;

        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;

    private:
        class Guts;
        Guts* _guts = nullptr;

        // Output start error: log error, detach channel & device, return false.
        bool startError(const UString& = UString(), unsigned int = 0); // Dtapi::DTAPI_RESULT

        // Update, when possible, the _opt_bitrate and _cur_bitrate fields based on a
        // user-specified symbol rate (and other modulation parameters). Return false
        // and close channel on error. Return true if the bitrate was successfully computed.
        bool computeBitrate(int symbol_rate, int dt_modulation, int param0, int param1, int param2);

        // Compute and display symbol rate (modulators only) if not explicitly specified by the user.
        void displaySymbolRate(const BitRate& ts_bitrate, int dt_modulation, int param0, int param1, int param2);

        // Set modulation parameters (modulators only). Return true on success, false on error.
        bool setModulation(int& modulation_type);

        // Set bitrate on the output channel.
        bool setBitrate(const BitRate& bitrate);

        // Set preload FIFO size based on a delay, if requested, in ms. Returns true if preload FIFO size is altered,
        // false otherwise.
        bool setPreloadFIFOSizeBasedOnDelay();

        // Checks whether calculated parameters for dvb-t do not override user specified params
        bool ParamsMatchUserOverrides(const ts::BitrateDifferenceDVBT& params);
    };
}

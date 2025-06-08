//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::DektecInputPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"

namespace ts {
    //!
    //! Dektec input plugin for @c tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDEKTECDLL DektecInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DektecInputPlugin);
    public:
        //! Destructor.
        virtual ~DektecInputPlugin() override;

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t stackUsage() const override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;

    private:
        class Guts;
        Guts* _guts = nullptr;

        // Start error: log error, detach channel & device, return false.
        bool startError(const UString&, unsigned int); // Dtapi::DTAPI_RESULT

        // Get the demodulation options as an XML string (debug mode only).
        UString demodParsToXml();

        // Configure the LNB. Return true on success.
        bool configureLNB();
    };
}

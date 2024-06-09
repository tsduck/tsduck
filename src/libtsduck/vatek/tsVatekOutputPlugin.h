//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2024, Vision Advance Technology Inc. (VATek)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::VatekOutputPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if !defined(TS_NO_VATEK) || defined(DOXYGEN)

#include "tsOutputPlugin.h"

namespace ts {
    //!
    //! Vatek output plugin for @c tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL VatekOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(VatekOutputPlugin);
    public:
        //! Destructor.
        virtual ~VatekOutputPlugin() override;

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
    };
}

#endif // TS_NO_VATEK

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HiDes modulator device output plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsHiDesDevice.h"

namespace ts {
    //!
    //! HiDes modulator device output plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL HiDesOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(HiDesOutputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;

    private:
        int             _dev_number = -1;  // Device adapter number.
        UString         _dev_name {};      // Device name.
        BitRate         _bitrate = 0;      // Nominal output bitrate.
        HiDesDevice     _device {};        // HiDes device object.
        HiDesDeviceInfo _dev_info {};      // HiDes device information.
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022, Richie Chang, Vision Advance Technology Inc. (VATek)
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
//!
//!  @file
//!  Declare the ts::VatekOutputPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#if !defined(TS_NO_VATEK) || defined(DOXYGEN)

#include "tsOutputPlugin.h"

TS_PUSH_WARNING()
TS_MSC_NOWARNING(5027)
TS_MSC_NOWARNING(4244)
#include <vatek_sdk_usbstream.h>
#include <core/base/output_modulator.h>
TS_POP_WARNING()

namespace ts {
    //!
    //! Vatek output plugin for @c tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL VatekOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(VatekOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        VatekOutputPlugin(TSP* tsp);

        //!
        //! Destructor.
        //!
        virtual ~VatekOutputPlugin() override;

        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;

    private:
        enum tsvatek_bandwidth {
            tsvatek_bw_1_7 = 0,
            tsvatek_bw_5   = 5,
            tsvatek_bw_6   = 6,
            tsvatek_bw_7   = 7,
            tsvatek_bw_8   = 8,
            tsvatek_bw_10  = 10,
        };

        typedef vatek_result (VatekOutputPlugin::*fpmodparam_config)(Pmodulator_param pmod);

        struct tsmod_param_config {
            modulator_type    type;
            fpmodparam_config config;
        };

        static const tsmod_param_config m_modtables[];

        hvatek_devices m_hdevices;
        hvatek_chip m_hchip;
        hvatek_usbstream m_husbstream;
        usbstream_param m_param;
        int32_t m_index;
        Pusbstream_slice m_slicebuf;

        void debugParams();
        vatek_result configParam();
        vatek_result modparam_config_dvb_t(Pmodulator_param pmod);
        vatek_result modparam_config_j83a(Pmodulator_param pmod);
        vatek_result modparam_config_atsc(Pmodulator_param pmod);
        vatek_result modparam_config_j83b(Pmodulator_param pmod);
        vatek_result modparam_config_dtmb(Pmodulator_param pmod);
        vatek_result modparam_config_isdb_t(Pmodulator_param pmod);
        vatek_result modparam_config_j83c(Pmodulator_param pmod);
        vatek_result modparam_config_dvb_t2(Pmodulator_param pmod);
    };
}

#endif // TS_NO_VATEK

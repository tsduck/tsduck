//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  DVB-S / DVB-S2 (satellite) tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"
#include "tsLNB.h"

namespace ts {
    //!
    //! DVB-S / DVB-S2 (satellite) tuners parameters.
    //! @ingroup hardware
    //!
    class TSDUCKDLL TunerParametersDVBS: public TunerParameters
    {
    public:
        // Public fields
        uint64_t          frequency;          //!< Satellite carrier frequency, in Hz.
        Polarization      polarity;           //!< Polarity (POL_HORIZONTAL or POL_VERTICAL only).
        LNB               lnb;                //!< Local dish LNB for frequency adjustment.
        SpectralInversion inversion;          //!< Spectral inversion, should be SPINV_AUTO..
        uint32_t          symbol_rate;        //!< Symbol rate.
        InnerFEC          inner_fec;          //!< Error correction.
        size_t            satellite_number;   //!< For DiSeqC (usually 0).
        DeliverySystem    delivery_system;    //!< Must be one of: DS_DVB_S, DS_DVB_S2.
        Modulation        modulation;         //!< QPSK for DVB-S, QPSK or PSK_8 for DVB-S2.
        Pilot             pilots;             //!< Presence of pilots (DVB-S2 only).
        RollOff           roll_off;           //!< Roll-off factor (DVB-S2 only).

        // Default values
        static const Polarization      DEFAULT_POLARITY         = POL_VERTICAL; //!< Default value for polarity.
        static const SpectralInversion DEFAULT_INVERSION        = SPINV_AUTO;   //!< Default value for inversion.
        static const uint32_t          DEFAULT_SYMBOL_RATE      = 27500000;     //!< Default value for symbol_rate.
        static const InnerFEC          DEFAULT_INNER_FEC        = FEC_AUTO;     //!< Default value for inner_fec.
        static const size_t            DEFAULT_SATELLITE_NUMBER = 0;            //!< Default value for satellite_number.
        static const DeliverySystem    DEFAULT_DELIVERY_SYSTEM  = DS_DVB_S;     //!< Default value for delivery_system.
        static const Modulation        DEFAULT_MODULATION       = QPSK;         //!< Default value for modulation.
        static const Pilot             DEFAULT_PILOTS           = PILOT_OFF;    //!< Default value for pilots.
        static const RollOff           DEFAULT_ROLL_OFF         = ROLLOFF_35;   //!< Default value for roll_off.
                                                                                //!< Implied value in DVB-S, default for DVB-S2.
        //!
        //! Default constructor.
        //!
        TunerParametersDVBS();

        // Implementation of TunerParameters API
        virtual BitRate theoreticalBitrate() const override;
        virtual UString shortDescription(int strength = -1, int quality = -1) const override;
        virtual UString toPluginOptions(bool no_local = false) const override;
        virtual xml::Element* toXML(xml::Element* parent) const override;
        virtual void displayParameters(std::ostream& strm, const UString& margin = UString(), bool verbose = false) const override;
        virtual void copy(const TunerParameters&) override;
        virtual bool convertToDektecModulation(int&, int&, int&, int&) const override;

    protected:
        virtual bool fromArgs(const TunerArgs&, Report&) override;
        virtual bool fromDeliveryDescriptor(const Descriptor& desc) override;
        virtual bool fromXML(const xml::Element* element) override;
    };
}

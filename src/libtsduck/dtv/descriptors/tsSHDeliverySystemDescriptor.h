//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Representation of a SH_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a SH_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SHDeliverySystemDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of a TDM modulation.
        //!
        struct TSDUCKDLL TDM
        {
            TDM();                    //!< Default constructor.
            uint8_t polarization;     //!< 2 bits, polarization.
            uint8_t roll_off;         //!< 2 bits, roll-off factor.
            uint8_t modulation_mode;  //!< 2 bits, modulation mode.
            uint8_t code_rate;        //!< 4 bits, code rate.
            uint8_t symbol_rate;      //!< 5 bits, encoded symbol rate.
        };

        //!
        //! Description of an OFDM modulation.
        //!
        struct TSDUCKDLL OFDM
        {
            OFDM();                              //!< Default constructor.
            uint8_t bandwidth;                   //!< 3 bits, bandwidth.
            uint8_t priority;                    //!< 1 bit, priority.
            uint8_t constellation_and_hierarchy; //!< 3 bits, polarization.
            uint8_t code_rate;                   //!< 4 bits, code rate.
            uint8_t guard_interval;              //!< 2 bits, guard interval.
            uint8_t transmission_mode;           //!< 2 bits, transmission mode.
            bool    common_frequency;            //!< 1 bit, common frequency.
        };

        //!
        //! Description of a modulation.
        //!
        struct TSDUCKDLL Modulation
        {
            Modulation();                  //!< Default constructor.
            bool    is_ofdm;               //!< Use tdm if false, ofdm if true.
            TDM     tdm;                   //!< TDM modulation.
            OFDM    ofdm;                  //!< OFDM modulation.
            bool    interleaver_presence;  //!< Use interleaver. If false, ignore all subsequent fields.
            bool    short_interleaver;     //!< If true, use only common_multiplier, ignore other fields.
            uint8_t common_multiplier;     //!< 6 bits, common multiplier.
            uint8_t nof_late_taps;         //!< 6 bits
            uint8_t nof_slices;            //!< 6 bits
            uint8_t slice_distance;        //!< 8 bits
            uint8_t non_late_increments;   //!< 6 bits
        };

        //!
        //! List of modulations.
        //!
        typedef std::list<Modulation> ModulationList;

        // SHDeliverySystemDescriptor public members:
        uint8_t        diversity_mode;   //!< 4 bits, diversity mode.
        ModulationList modulations;      //!< List of modulations.

        //!
        //! Default constructor.
        //!
        SHDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SHDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Enumerations for XML and display.
        static const Enumeration BandwidthNames;
        static const Enumeration GuardIntervalNames;
        static const Enumeration TransmissionModeNames;
        static const Enumeration PolarizationNames;
        static const Enumeration RollOffNames;
        static const Enumeration ModulationNames;
    };
}

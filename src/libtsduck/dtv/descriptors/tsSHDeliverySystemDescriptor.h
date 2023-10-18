//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            TDM() = default;              //!< Default constructor.
            uint8_t polarization = 0;     //!< 2 bits, polarization.
            uint8_t roll_off = 0;         //!< 2 bits, roll-off factor.
            uint8_t modulation_mode = 0;  //!< 2 bits, modulation mode.
            uint8_t code_rate = 0;        //!< 4 bits, code rate.
            uint8_t symbol_rate = 0;      //!< 5 bits, encoded symbol rate.
        };

        //!
        //! Description of an OFDM modulation.
        //!
        struct TSDUCKDLL OFDM
        {
            OFDM() = default;                        //!< Default constructor.
            uint8_t bandwidth = 0;                   //!< 3 bits, bandwidth.
            uint8_t priority = 0;                    //!< 1 bit, priority.
            uint8_t constellation_and_hierarchy = 0; //!< 3 bits, polarization.
            uint8_t code_rate = 0;                   //!< 4 bits, code rate.
            uint8_t guard_interval = 0;              //!< 2 bits, guard interval.
            uint8_t transmission_mode = 0;           //!< 2 bits, transmission mode.
            bool    common_frequency = false;        //!< 1 bit, common frequency.
        };

        //!
        //! Description of a modulation.
        //!
        struct TSDUCKDLL Modulation
        {
            Modulation() = default;                //!< Default constructor.
            bool    is_ofdm = false;               //!< Use tdm if false, ofdm if true.
            TDM     tdm {};                        //!< TDM modulation.
            OFDM    ofdm {};                       //!< OFDM modulation.
            bool    interleaver_presence = false;  //!< Use interleaver. If false, ignore all subsequent fields.
            bool    short_interleaver = false;     //!< If true, use only common_multiplier, ignore other fields.
            uint8_t common_multiplier = 0;         //!< 6 bits, common multiplier.
            uint8_t nof_late_taps = 0;             //!< 6 bits
            uint8_t nof_slices = 0;                //!< 6 bits
            uint8_t slice_distance = 0;            //!< 8 bits
            uint8_t non_late_increments = 0;       //!< 6 bits
        };

        //!
        //! List of modulations.
        //!
        typedef std::list<Modulation> ModulationList;

        // SHDeliverySystemDescriptor public members:
        uint8_t        diversity_mode = 0;   //!< 4 bits, diversity mode.
        ModulationList modulations {};       //!< List of modulations.

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

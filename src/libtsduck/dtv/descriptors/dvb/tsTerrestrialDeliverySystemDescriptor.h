//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a terrestrial_delivery_system_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of a terrestrial_delivery_system_descriptor.
    //! @see ETSI EN 300 468, 6.2.13.4.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TerrestrialDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // TerrestrialDeliverySystemDescriptor public members:
        uint64_t centre_frequency = 0;     //!< Frequency in Hz (warning: coded in 10 Hz units in descriptor).
        uint8_t  bandwidth = 0;            //!< Bandwidth, 0..7 (3 bits).
        bool     high_priority = true;     //!< Must be true if hierarchy == 0.
        bool     no_time_slicing = true;   //!< No time slicing.
        bool     no_mpe_fec = true;        //!< NO MPE-FEC.
        uint8_t  constellation = 0;        //!< Constellation, 0..3 (2 bits).
        uint8_t  hierarchy = 0;            //!< Hierarchy, 0..7 (3 bits).
        uint8_t  code_rate_hp = 0;         //!< Code Rate, high priority, 0..7 (3 bits).
        uint8_t  code_rate_lp = 0;         //!< Code Rate, low priority, 0..7 (3 bits).
        uint8_t  guard_interval = 0;       //!< Guard interval, 0..3 (2 bits).
        uint8_t  transmission_mode = 0;    //!< Transmission mode, 0..3 (2 bits).
        bool     other_frequency = false;  //!< Other frequency.

        //!
        //! Default constructor.
        //!
        TerrestrialDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TerrestrialDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Translate the binary value in bandwidth as a BandWidth value in Hz.
        //! @return The corresponding BandWidth value in Hz.
        //!
        BandWidth getBandwidth() const { return translate(bandwidth, ToBandWidth(), BandWidth(0)); }

        //!
        //! Translate the binary value in constellation as a Modulation enumeration value.
        //! @return The corresponding Modulation enumeration value.
        //!
        Modulation getConstellation() const { return translate(constellation, ToConstellation(), QAM_AUTO); }

        //!
        //! Translate the binary value in code_rate_hp as a InnerFEC enumeration value.
        //! @return The corresponding InnerFEC enumeration value.
        //!
        InnerFEC getCodeRateHP() const { return translate(code_rate_hp, ToInnerFEC(), FEC_AUTO); }

        //!
        //! Translate the binary value in code_rate_lp as a InnerFEC enumeration value.
        //! @return The corresponding InnerFEC enumeration value.
        //!
        InnerFEC getCodeRateLP() const { return translate(code_rate_lp, ToInnerFEC(), FEC_AUTO); }

        //!
        //! Translate the binary value in transmission_mode as a TransmissionMode enumeration value.
        //! @return The corresponding TransmissionMode enumeration value.
        //!
        TransmissionMode getTransmissionMode() const { return translate(transmission_mode, ToTransmissionMode(), TM_AUTO); }

        //!
        //! Translate the binary value in guard_interval as a GuardInterval enumeration value.
        //! @return The corresponding GuardInterval enumeration value.
        //!
        GuardInterval getGuardInterval() const { return translate(guard_interval, ToGuardInterval(), GUARD_AUTO); }

        //!
        //! Translate the binary value in hierarchy as a Hierarchy enumeration value.
        //! @return The corresponding Hierarchy enumeration value.
        //!
        Hierarchy getHierarchy() const { return translate(hierarchy, ToHierarchy(), HIERARCHY_AUTO); }

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Thread-safe init-safe static data patterns.
        static const Enumeration& BandwidthNames();
        static const Enumeration& PriorityNames();
        static const Enumeration& ConstellationNames();
        static const Enumeration& CodeRateNames();
        static const Enumeration& GuardIntervalNames();
        static const Enumeration& TransmissionModeNames();
        static const std::map<int, BandWidth>& ToBandWidth();
        static const std::map<int, Modulation>& ToConstellation();
        static const std::map<int, InnerFEC>& ToInnerFEC();
        static const std::map<int, TransmissionMode>& ToTransmissionMode();
        static const std::map<int, GuardInterval>& ToGuardInterval();
        static const std::map<int, Hierarchy>& ToHierarchy();
    };
}

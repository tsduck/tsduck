//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB wired_multicarrier_transmission_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB wired_multicarrier_transmission_descriptor
    //! @see JCTEA STD-003, 6.2 J3
    //! @ingroup libtsduck descriptor
    //!
    //! Also named:
    //! - wired_multicarrier_transmission_distribution_system_descriptor (ARIB STD-B10)
    //! - channel_bonding_cable_delivery_system_descriptor (JCTEA STD-003)
    //!
    //! The JCTEA documents are not publicly accessible. This descriptor is
    //! defined as follow in JCTEA STD-003:
    //!
    //! @code
    //! Syntax                           Bits  Identifier
    //! -------------------------------  ----  -------------
    //! Channel_bonding_cable_delivery_system_descriptor() {
    //!     descriptor_tag                  8  uimsbf
    //!     descriptor_length               8  uimsbf
    //!     for (i=0; i<N; i++) {
    //!         frequency                  32  bslbf
    //!         reserved_future_use         8  bslbf
    //!         frame_type                  4  uimsbf
    //!         FEC_outer                   4  bslbf
    //!         modulation                  8  bslbf
    //!         symbol_rate                28  bslbf
    //!         FEC_inner                   4  bslbf
    //!         group_id                    8  bslbf
    //!     }
    //! }
    //! @endcode
    //!
    //! frequency: This is a 32-bit field that represents the frequency as 8 digits of a 4-bit BCD code.
    //! In the cable distribution system descriptor, the frequency is coded in MHz, counting from the
    //! most significant digit to the decimal point after the 4th digit.
    //! Example: 0733.0000 MHz -> 0x07330000
    //!
    //! symbol rate: This is a 28-bit field, a 7-digit 4-bit BCD code, with the symbol rate value in
    //! Msymbol/s, with the decimal point after the third digit starting from the most significant digit.
    //! Example: 5.274 Msymbol/s -> 0x0052740
    //!
    //! FEC inner: This is a 4-bit field that indicates the inner code. The value of this field shall be '1111'.
    //!
    //! group id (carrier group identification): The carrier group identification is an 8-bit field that
    //! indicates information that identifies the carrier group that transmits the multiplexed frame using
    //! the extended information, and the value 0x00 shall not be used.
    //!
    class TSDUCKDLL ISDBWiredMultiCarrierTransmissionDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of one carrier frequency.
        //!
        class TSDUCKDLL Carrier
        {
        public:
            uint64_t frequency = 0;          //!< Frequency in Hz (warning: coded in 100 Hz units in descriptor).
            uint8_t  frame_type = 0;         //!< Frame type, 4 bits.
            uint8_t  FEC_outer = 0;          //!< FEC outer, 4 bits.
            uint8_t  modulation = 0;         //!< Modulation, 8 bits.
            uint64_t symbol_rate = 0;        //!< Symbol rate (warning: coded in 100 symbols/second units in descriptor).
            uint8_t  FEC_inner = 0x0F;       //!< FEC inner, 4 bits, value must be all ones.
            uint8_t  group_id = 0;           //!< Group id, 8 bits.
        };

        // ISDBWiredMultiCarrierTransmissionDescriptor public members:
        std::list<Carrier> carriers {};  //!< List of carriers.

        //!
        //! Default constructor.
        //!
        ISDBWiredMultiCarrierTransmissionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBWiredMultiCarrierTransmissionDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an S2Xv2_satellite_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an S2Xv2_satellite_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.5.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL S2Xv2SatelliteDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // S2Xv2SatelliteDeliverySystemDescriptor public members:
        uint32_t                delivery_system_id = 0;                    //!< 32 bits
        uint8_t                 S2Xv2_mode = 0;                            //!< 4 bits, S2Xv2 mode.
        bool                    multiple_input_stream_flag = false;        //!< 1 bit
        uint8_t                 roll_off = 0;                              //!< 3 bits, roll-off factor.
        uint8_t                 NCR_version = 0;                           //!< 1 bit
        uint8_t                 channel_bond = 0;                          //!< 2 bits
        uint8_t                 polarization = 0;                          //!< 2 bits, polarization.
        uint8_t                 TS_GS_S2X_mode = 0;                        //!< 2 bits, TS-GS S2X mode.
        uint8_t                 receiver_profiles = 0;                     //!< 5 bits, receiver_profiles bit mask.
        uint32_t                satellite_id = 0;                          //!< 24 bits
        uint64_t                frequency = 0;                             //!< Frequency in GHz. (%05d.%03d)
        uint64_t                symbol_rate = 0;                           //!< Symbol rate in Msymbols/second. (%04d.%04d)
        uint8_t                 input_stream_identifier = 0;               //!< 8 bits
        std::optional<uint32_t> scrambling_sequence_index {};              //!< 18 bits
        uint8_t                 timeslice_number = 0;                      //!< 8 bits
        uint8_t                 num_channel_bonds_minus1 = 0;              //!< 1 bit
        std::vector<uint32_t>   secondary_delivery_system_ids {};          //!< 32 bits
        uint8_t                 SOSF_WH_sequence_number = 0;               //!< 8 bits
        uint32_t                reference_scrambling_index = 0;            //!< 20 bits
        std::optional<uint8_t>  SFFI {};                                   //!< 4 bits
        uint32_t                payload_scrambling_index = 0;              //!< 20 bits
        std::optional<uint32_t> beamhopping_time_plan_id {};               //!< 32 bits
        uint8_t                 superframe_pilots_WH_sequence_number = 0;  //!< 5 bits
        ByteBlock               reserved_future_use {};                    //!< For future modes.

        //!
        //! Default constructor.
        //!
        S2Xv2SatelliteDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        S2Xv2SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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
    };
}

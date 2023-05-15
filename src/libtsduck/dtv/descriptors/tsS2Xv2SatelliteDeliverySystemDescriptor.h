//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
        uint32_t                delivery_system_id;                     //!< 32 bits
        uint8_t                 S2Xv2_mode;                             //!< 4 bits, S2Xv2 mode.
        bool                    multiple_input_stream_flag;             //!< 1 bit
        uint8_t                 roll_off;                               //!< 3 bits, roll-off factor.
        uint8_t                 NCR_version;                            //!< 1 bit
        uint8_t                 channel_bond;                           //!< 2 bits
        uint8_t                 polarization;                           //!< 2 bits, polarization.
        uint8_t                 TS_GS_S2X_mode;                         //!< 2 bits, TS-GS S2X mode.
        uint8_t                 receiver_profiles;                      //!< 5 bits, receiver_profiles bit mask.
        uint32_t                satellite_id;                           //!< 24 bits
        uint64_t                frequency;                              //!< Frequency in GHz. (%05d.%03d)
        uint64_t                symbol_rate;                            //!< Symbol rate in Msymbols/second. (%04d.%04d)
        uint8_t                 input_stream_identifier;                //!< 8 bits
        Variable<uint32_t>      scrambling_sequence_index;              //!< 18 bits
        uint8_t                 timeslice_number;                       //!< 8 bits
        uint8_t                 num_channel_bonds_minus1;               //!< 1 bit
        std::vector<uint32_t>   secondary_delivery_system_ids;          //!< 32 bits
        uint8_t                 SOSF_WH_sequence_number;                //!< 8 bits
        uint32_t                reference_scrambling_index;             //!< 20 bits
        Variable<uint8_t>       SFFI;                                   //!< 4 bits
        uint32_t                payload_scrambling_index;               //!< 20 bits
        Variable<uint32_t>      beamhopping_time_plan_id;               //!< 32 bits
        uint8_t                 superframe_pilots_WH_sequence_number;   //!< 5 bits
        ByteBlock               reserved_future_use;                    //!< For future modes.

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

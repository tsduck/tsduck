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
//!  Representation of an S2X_satellite_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an S2X_satellite_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL S2XSatelliteDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        //!
        //! Description of a channel.
        //! There is one master channel and up to 2 bonded channels.
        //!
        class TSDUCKDLL Channel
        {
        public:
            Channel();                            //!< Default constructor.
            void clear();                         //!< Clear content.

            uint64_t frequency;                   //!< Frequency in Hz.
            uint16_t orbital_position;            //!< Orbital position, unit is 0.1 degree.
            bool     east_not_west;               //!< True for East, false for West.
            uint8_t  polarization;                //!< 2 bits, polarization.
            uint8_t  roll_off;                    //!< 3 bits, roll-off factor.
            uint64_t symbol_rate;                 //!< Symbol rate in symbols/second.
            bool     multiple_input_stream_flag;  //!< True when input_stream_identifier is valid.
            uint8_t  input_stream_identifier;     //!< Input stream identifier.
        };

        // S2XSatelliteDeliverySystemDescriptor public members:
        uint8_t   receiver_profiles;             //!< 5 bits, receiver_profiles bit mask.
        uint8_t   S2X_mode;                      //!< 2 bits, S2X mode.
        uint8_t   TS_GS_S2X_mode;                //!< 2 bits, TS-GS S2X mode.
        bool      scrambling_sequence_selector;  //!< True when scrambling_sequence_index is valid.
        uint32_t  scrambling_sequence_index;     //!< 18 bits, scrambling sequence index.
        uint8_t   timeslice_number;              //!< Time slice number, valid when S2X_mode==2.
        Channel   master_channel;                //!< Master channel
        bool      num_channel_bonds_minus_one;   //!< When true, use two channel bonds.
        Channel   channel_bond_0;                //!< First channel bond, valid when S2X_mode==3.
        Channel   channel_bond_1;                //!< Second channel bond, valid when S2X_mode==3 and num_channel_bonds_minus_one is true.
        ByteBlock reserved_future_use;           //!< For future modes.

        //!
        //! Default constructor.
        //!
        S2XSatelliteDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        S2XSatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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
        friend class S2Xv2SatelliteDeliverySystemDescriptor;
        // Enumerations for XML.
        static const Enumeration RollOffNames;

        // Serialization / deserialization of a channel description.
        void serializeChannel(const Channel&, PSIBuffer&) const;
        void deserializeChannel(Channel&, PSIBuffer&);
        void buildChannelXML(const Channel&, xml::Element* parent, const UString& name) const;
        bool getChannelXML(Channel&, DuckContext&, const xml::Element*);
        static void DisplayChannel(TablesDisplay&, const UString& title, PSIBuffer&, const UString& margin);
    };
}

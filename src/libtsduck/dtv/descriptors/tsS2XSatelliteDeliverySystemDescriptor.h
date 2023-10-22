//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            Channel() = default;                          //!< Default constructor.
            void clear();                                 //!< Clear content.
            uint64_t frequency = 0;                       //!< Frequency in Hz.
            uint16_t orbital_position = 0;                //!< Orbital position, unit is 0.1 degree.
            bool     east_not_west = false;               //!< True for East, false for West.
            uint8_t  polarization = 0;                    //!< 2 bits, polarization.
            uint8_t  roll_off = 0;                        //!< 3 bits, roll-off factor.
            uint64_t symbol_rate = 0;                     //!< Symbol rate in symbols/second.
            bool     multiple_input_stream_flag = false;  //!< True when input_stream_identifier is valid.
            uint8_t  input_stream_identifier = 0;         //!< Input stream identifier.
        };

        // S2XSatelliteDeliverySystemDescriptor public members:
        uint8_t   receiver_profiles = 0;                 //!< 5 bits, receiver_profiles bit mask.
        uint8_t   S2X_mode = 0;                          //!< 2 bits, S2X mode.
        uint8_t   TS_GS_S2X_mode = 0;                    //!< 2 bits, TS-GS S2X mode.
        bool      scrambling_sequence_selector = false;  //!< True when scrambling_sequence_index is valid.
        uint32_t  scrambling_sequence_index = 0;         //!< 18 bits, scrambling sequence index.
        uint8_t   timeslice_number = 0;                  //!< Time slice number, valid when S2X_mode==2.
        Channel   master_channel {};                     //!< Master channel
        bool      num_channel_bonds_minus_one {};        //!< When true, use two channel bonds.
        Channel   channel_bond_0 {};                     //!< First channel bond, valid when S2X_mode==3.
        Channel   channel_bond_1 {};                     //!< Second channel bond, valid when S2X_mode==3 and num_channel_bonds_minus_one is true.
        ByteBlock reserved_future_use {};                //!< For future modes.

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

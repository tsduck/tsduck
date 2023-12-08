//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB cpcm_delivery_signalling_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a DVB cpcm_delivery_signalling_descriptor.
    //! @see ETSI TS 102 825-9, clause 4.1.5 and ETSI TS 102 825-4 clause 5.4.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CPCMDeliverySignallingDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Vector of CPS.
        //!
        class TSDUCKDLL CPSvector {
        public:
            CPSvector() = default;              //!< Constructor.
            uint8_t   C_and_R_regime_mask = 0;  //!< ETSI TS 102 825-4, clause 5.4.5
            ByteBlock cps_byte {};              //!< ETSI TS 102 825-4, clause 5.4.5
        };

        //!
        //! CPCM version 1.
        //! @see ETSI TS 102 825-4, clause 5.4.5
        //!
        class TSDUCKDLL CPCMv1Signalling {
        public:
            uint8_t                 copy_control = 0;                          //!< 3 bits, ETSI TS 102 825-4, clause 5.4.5
            bool                    do_not_cpcm_scramble = false;              //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    viewable = false;                          //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    move_local = false;                        //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    view_local = false;                        //!< flag, ETSI TS 102 825-4, clause 5.4.5
            uint8_t                 move_and_copy_propagation_information = 0; //!< 2 bits, ETSI TS 102 825-4, clause 5.4.5
            uint8_t                 view_propagation_information = 0;          //!< 2 bits, ETSI TS 102 825-4, clause 5.4.5
            bool                    remote_access_record_flag = false;         //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    export_beyond_trust = false;               //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    disable_analogue_sd_export = false;        //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    disable_analogue_sd_consumption = false;   //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    disable_analogue_hd_export = false;        //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    disable_analogue_hd_consumption = false;   //!< flag, ETSI TS 102 825-4, clause 5.4.5
            bool                    image_constraint = false;                  //!< flag, ETSI TS 102 825-4, clause 5.4.5
            std::optional<Time>          view_window_start {};                      //!< 40 bits, ETSI TS 102 825-4, clause 5.4.5
            std::optional<Time>          view_window_end {};                        //!< 40 bits, ETSI TS 102 825-4, clause 5.4.5
            std::optional<uint16_t>      view_period_from_first_playback {};        //!< 16 bits, ETSI TS 102 825-4, clause 5.4.5
            std::optional<uint8_t>       simultaneous_view_count {};                //!< 8 bits, ETSI TS 102 825-4, clause 5.4.5
            std::optional<uint16_t>      remote_access_delay {};                    //!< 16 bits, ETSI TS 102 825-4, clause 5.4.5
            std::optional<Time>          remote_access_date {};                     //!< 40 bits, ETSI TS 102 825-4, clause 5.4.5
            std::vector<CPSvector>  cps_vector {};                             //!< ETSI TS 102 825-4, clause 5.4.5

            CPCMv1Signalling() = default;                                      //!< Default constructor.
            void clearContent();                                               //!< Reset state.

            //!
            //! Serialize the structure to binary.
            //! @param [in,out] buf Serialization buffer.
            //!
            void serializePayload(PSIBuffer& buf) const;

            //!
            //! Deserialize the structure from binary.
            //! @param [in,out] buf Deserialization buffer.
            //!
            void deserializePayload(PSIBuffer& buf);
        };

        // Public members:
        uint8_t          cpcm_version = 0;                //!< 8 bits, ETSI TS 102 825-9, clause 4.1.5
        CPCMv1Signalling cpcm_v1_delivery_signalling {};  //!< ETSI TS 102 825-4, clause 5.4.5

        //!
        //! Default constructor.
        //!
        CPCMDeliverySignallingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CPCMDeliverySignallingDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB advanced_cable_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB advanced_cable_delivery_system_descriptor.
    //! @see JCTEA STD-003, 6.2
    //! @ingroup libtsduck descriptor
    //!
    //! Unlike other ISDB descriptors, this one is not fully documented in JCTEA STD-003.
    //! It only appears in a block diagram. The following syntax has been rebuilt from
    //! this block diagram.
    //!
    //! More testing is required against real signalization because this definition,
    //! although valid is suspicious: 8-bit data are uselessly unaligned.
    //!
    //! @code
    //! Syntax                                  Bits  Identifier
    //! --------------------------------------  ----  -------------
    //! advanced_cable_delivery_system_descriptor() {
    //!     descriptor_tag                         8  uimsbf
    //!     descriptor_length                      8  uimsbf
    //!     descriptor_tag_extension               8  uimsbf
    //!     if (descriptor_tag_extension == 0x00) {
    //!         // normal data transmission
    //!         plp_id                             8  uimsbf
    //!         effective_symbol_length            3  bslbf
    //!         guard_interval                     3  bslbf
    //!         bundled_channel                    8  uimsbf
    //!         reserved_for_future_use            2  bslbf
    //!         for (i=0; i<N; i++) {
    //!             data_slice_id                  8  uimsbf
    //!             frequency                     32  bslbf
    //!             frame_type                     2  bslbf
    //!             FEC_outer                      4  bslbf
    //!             modulation                     8  bslbf
    //!             FEC_inner                      4  bslbf
    //!             reserved_for_future_use        6  bslbf
    //!         }
    //!     }
    //!     else if (descriptor_tag_extension == 0x01) {
    //!         // earthquake warning information transmission
    //!         earthquake_warning_information   204  bslbf   // 25.5 bytes
    //!         reserved_for_future_use          500  bslbf   // 62.5 bytes
    //!     }
    //! }
    //! @endcode
    //!
    class TSDUCKDLL ISDBAdvancedCableDeliverySystemDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of one carrier frequency.
        //!
        class TSDUCKDLL Carrier
        {
        public:
            uint8_t  data_slice_id = 0;  //!< Data slice id, 8 bits.
            uint64_t frequency = 0;      //!< Frequency in Hz (warning: coded in 100 Hz units in descriptor).
            uint8_t  frame_type = 0;     //!< Frame type, 2 bits.
            uint8_t  FEC_outer = 0;      //!< FEC outer, 4 bits.
            uint8_t  modulation = 0;     //!< Modulation, 8 bits.
            uint8_t  FEC_inner = 0;      //!< FEC inner, 4 bits.
        };

        //!
        //! Definition of "normal data transmission", when descriptor_tag_extension is 0x00.
        //!
        class TSDUCKDLL NormalData
        {
        public:
            uint8_t plp_id = 0;                   //!< 8 bits.
            uint8_t effective_symbol_length = 0;  //!< 3 bits.
            uint8_t guard_interval = 0;           //!< 3 bits.
            uint8_t bundled_channel = 0;          //!< 8 bits.
            std::list<Carrier> carriers {};       //!< List of carriers.
        };

        // ISDBAdvancedCableDeliverySystemDescriptor public members:
        uint8_t    descriptor_tag_extension = 0;  //!< Extension type, 8 bits.
        NormalData normal_data {};                //!< When descriptor_tag_extension == 0x00.
        ByteBlock  other_data {};                 //!< When descriptor_tag_extension != 0x00.

        //!
        //! Default constructor.
        //!
        ISDBAdvancedCableDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBAdvancedCableDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB material_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB material_information_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.6
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL MaterialInformationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of one material.
        //!
        class TSDUCKDLL Material
        {
        public:
            uint8_t                    material_type = 0;       //!< Material type.
            UString                    material_name {};        //!< Material name.
            uint8_t                    material_code_type = 0;  //!< Material code type.
            UString                    material_code {};        //!< Material code.
            std::optional<cn::seconds> material_period {};      //!< hh:mm:ss
            uint8_t                    material_url_type = 0;   //!< Material URL type.
            UString                    material_url {};         //!< Material URL.
            ByteBlock                  reserved {};             //!< For future use.
        };

        // MaterialInformationDescriptor public members:
        uint8_t             descriptor_number = 0;       //!< 4 bits.
        uint8_t             last_descriptor_number = 0;  //!< 4 bits.
        std::list<Material> material {};                 //!< List of materials.

        //!
        //! Default constructor.
        //!
        MaterialInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MaterialInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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

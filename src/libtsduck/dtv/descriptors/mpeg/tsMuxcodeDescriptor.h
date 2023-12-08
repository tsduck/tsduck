//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MuxCode_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MuxCode descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.48 and ISO/IEC 14496-1, 7.4.2.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MuxCodeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Mux code substructure.
        //!
        class TSDUCKDLL substructure_type {
        public:
            substructure_type() = default;             //!< Constructor
            uint8_t              repititionCount = 0;  //!< 3 bits
            std::vector<uint8_t> m4MuxChannel {};      //!< list of 8 bit values
            std::vector<uint8_t> numberOfBytes {};     //!< list of 8 bit values
        };

        //!
        //! One mux code table entry.
        //!
        class TSDUCKDLL MuxCodeTableEntry_type {
        public:
            MuxCodeTableEntry_type() = default;              //!< Constructor
            uint8_t                        MuxCode = 0;      //!< 4 bits
            uint8_t                        version = 0;      //!< 4 bits
            std::vector<substructure_type> substructure {};  //!< Mux code substructure
        };

        // Public members:
        std::vector<MuxCodeTableEntry_type> MuxCodeTableEntry {};  //!< All mux codes.

        //!
        //! Default constructor.
        //!
        MuxCodeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MuxCodeDescriptor(DuckContext& duck, const Descriptor& bin);

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

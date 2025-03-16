//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Downloadable Font Information Table (DFIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of a Downloadable Font Information Table (DFIT)
    //! @see ETSI EN 303 560, 5.3.2.3
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL DFIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of font style and weight.
        //!
        class TSDUCKDLL FontStyleWeight
        {
        public:
            uint8_t font_style = 0;   //!< 3 bits.
            uint8_t font_weight = 0;  //!< 4 bits.
        };

        //!
        //! Description of a font file.
        //!
        class TSDUCKDLL FontFile
        {
        public:
            uint8_t font_file_format = 0;  //!< 4 bits.
            UString uri {};                //!< Font file URI.
        };

        // DFIT public members:
        uint16_t                     font_id_extension = 0;  //!< Font id extension, 9 bits, usually all zeroes.
        uint8_t                      font_id = 0;            //!< Font id, 7 bits.
        std::vector<FontStyleWeight> font_style_weight {};   //!< 1 or more.
        std::vector<FontFile>        font_file_URI {};       //!< 1 or more.
        std::vector<uint16_t>        font_size {};           //!< size in pixels, 0 or more.
        UString                      font_family {};         //!< Exacly one.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        DFIT(uint8_t vers = 0, bool cur = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DFIT(const DFIT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DFIT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

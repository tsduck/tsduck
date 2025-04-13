//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Aggregate Extended Text Table (AETT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsATSCMultipleString.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ATSC Aggregate Extended Text Table (AETT)
    //! @see ATSC A/81, section 9.9.3.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL AETT : public AbstractLongTable
    {
    public:
        //!
        //! Description of an extended text message.
        //!
        class TSDUCKDLL ETM
        {
        public:
            ETM() = default;                              //!< Constructor.
            uint32_t           ETM_id = 0;                //!< Extended text message id.
            ATSCMultipleString extended_text_message {};  //!< Extended text message.
        };

        //!
        //! List of extended text messages.
        //!
        using ETMList = std::list<ETM>;

        // AETT public members:
        uint8_t   AETT_subtype = 0;  //!< AETT format, only 0 is defined.
        uint8_t   MGT_tag = 0;       //!< Table type in MGT.
        ETMList   etms {};           //!< List of extended text messages, when AETT_subtype == 0.
        ByteBlock reserved {};       //!< Reserved data, when AETT_subtype != 0.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        AETT(uint8_t version = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        AETT(DuckContext& duck, const BinaryTable& table);

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

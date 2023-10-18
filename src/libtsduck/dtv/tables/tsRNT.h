//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB Resolution provider Notification Table (RNT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of a DVB Resolution provider Notification Table (RNT).
    //! @see ETSI TS 102 323, 5.2.2
    //! @ingroup table
    //!
    class TSDUCKDLL RNT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a CRID autority.
        //! Note: by inheriting from EntryWithDescriptors, this class has a @c descs fields.
        //!
        class TSDUCKDLL CRIDAuthority: public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(CRIDAuthority);
            TS_DEFAULT_ASSIGMENTS(CRIDAuthority);
        public:
            UString name {};     //!< CRID authority name.
            uint8_t policy = 0;  //!< 2 bits, CRID authority policy.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit CRIDAuthority(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            CRIDAuthority(const AbstractTable* table, const CRIDAuthority& other);
        };

        //!
        //! List of CRID autorities.
        //!
        typedef EntryWithDescriptorsList<CRIDAuthority> CRIDAutorityList;

        //!
        //! Description of a resolution provider.
        //! Note: by inheriting from EntryWithDescriptors, this class has a @c descs fields.
        //!
        class TSDUCKDLL ResolutionProvider: public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(ResolutionProvider);
            TS_DEFAULT_ASSIGMENTS(ResolutionProvider);
        public:
            UString          name {};           //!< Resolution provider name.
            CRIDAutorityList CRID_authorities;  //!< List of CRID autorities.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit ResolutionProvider(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            ResolutionProvider(const AbstractTable* table, const ResolutionProvider& other);
        };

        //!
        //! List of resolution providers.
        //!
        typedef EntryWithDescriptorsList<ResolutionProvider> ResolutionProviderList;

        // RNT public members:
        uint16_t               context_id = 0;       //!< Network or bouquet id.
        uint8_t                context_id_type = 0;  //!< Type of content in context_id.
        DescriptorList         descs;                //!< Top-level descriptor list.
        ResolutionProviderList providers;            //!< List of resolution providers.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        RNT(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        RNT(DuckContext& duck, const BinaryTable& table);

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

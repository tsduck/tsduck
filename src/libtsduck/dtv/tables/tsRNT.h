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
        public:
            UString name;    //!< CRID authority name.
            uint8_t policy;  //!< 2 bits, CRID authority policy.

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

            //! @cond nodoxygen
            CRIDAuthority& operator=(const CRIDAuthority& other) = default;
            CRIDAuthority& operator=(CRIDAuthority&& other) = default;
            //! @endcond nodoxygen
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
        public:
            UString          name;              //!< Resolution provider name.
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

            //! @cond nodoxygen
            ResolutionProvider& operator=(const ResolutionProvider& other) = default;
            ResolutionProvider& operator=(ResolutionProvider&& other) = default;
            //! @endcond nodoxygen
        };

        //!
        //! List of resolution providers.
        //!
        typedef EntryWithDescriptorsList<ResolutionProvider> ResolutionProviderList;

        // RNT public members:
        uint16_t               context_id;       //!< Network or bouquet id.
        uint8_t                context_id_type;  //!< Type of content in context_id.
        DescriptorList         descs;            //!< Top-level descriptor list.
        ResolutionProviderList providers;        //!< List of resolution providers.

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

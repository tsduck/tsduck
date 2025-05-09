//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Network Resources Table (NRT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDSMCCCompatibilityDescriptor.h"
#include "tsDSMCCResourceDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC Network Resources Table (NRT)
    //! @see ATSC A/90, section 12.3.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL NRT : public AbstractLongTable
    {
        TS_DEFAULT_COPY_MOVE(NRT);
    public:
        //!
        //! Description of a resource.
        //!
        class TSDUCKDLL Resource
        {
        public:
            Resource() = default;                                      //!< Default constructor.
            DSMCCCompatibilityDescriptor compatibility_descriptor {};  //!< DSM-CC compatibilityDescriptor() structure.
            DSMCCResourceDescriptor      resource_descriptor {};       //!< DSM-CC dsmccResourceDescriptor() structure.
        };

        // NRT public members:
        uint16_t            table_id_extension = 0xFFFF;  //!< ATSC reserved.
        std::list<Resource> resources {};                 //!< List of resources.
        ByteBlock           private_data {};              //!< Private data.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        NRT(uint8_t version = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        NRT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual bool isCurrent() const override;
        virtual void setCurrent(bool is_current) override;
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // In a NRT, current is always true.
        static constexpr bool CURRENT = true;
    };
}

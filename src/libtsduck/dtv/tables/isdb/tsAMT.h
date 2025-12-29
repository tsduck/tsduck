//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Address Map Table (AMT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsIPAddressMask.h"

namespace ts {
    //!
    //! Representation of an ISDB Address Map Table (AMT).
    //! @see ARIB STD-B10, Part 2, 5.2.16
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL AMT : public AbstractLongTable
    {
    public:
        //!
        //! Service entry.
        //!
        class TSDUCKDLL Service
        {
        public:
            IPAddressMask src {};           //!< Source address and mask.
            IPAddressMask dst {};           //!< Destination address and mask.
            ByteBlock     private_data {};  //!< Private data for that service.

            //!
            //! Evaluate the binary size of the service entry.
            //! @return The binary size, in bytes, of the service entry.
            //!
            size_t binarySize() const;
        };

        //!
        //! Map of service entries, indexed by service id.
        //!
        using ServiceMap = std::map<uint16_t, Service>;

        // AMT public members:
        ServiceMap services {};   //!< Map of service entries, indexed by service id.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        AMT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        AMT(DuckContext& duck, const BinaryTable& table);

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

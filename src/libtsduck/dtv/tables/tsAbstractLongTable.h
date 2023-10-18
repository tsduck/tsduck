//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables with long sections
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI tables with long sections.
    //! @ingroup table
    //!
    class TSDUCKDLL AbstractLongTable: public AbstractTable
    {
        TS_RULE_OF_FIVE(AbstractLongTable, override);
    public:
        // Common public members:
        uint8_t version = 0;        //!< Table version number.
        bool    is_current = true;  //!< True if table is current, false if table is next.

        //!
        //! Get the table id extension.
        //! The table id extension is a 16-bit field which usually contains one of the
        //! table fields (service id, transport stream id, etc.) For each subclass, the
        //! table id extension is usually directly available in the corresponding public
        //! field. This virtual method is a generic way to access the table id extension.
        //! @return The table id extension.
        //!
        virtual uint16_t tableIdExtension() const = 0;

        // Inherited methods
        virtual void clear() override final;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] version_ Table version number.
        //! @param [in] is_current_ True if table is current, false if table is next.
        //!
        AbstractLongTable(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_);

        // Inherited methods.
        virtual size_t maxPayloadSize() const override;
        virtual bool useTrailingCRC32() const override;
        virtual void deserializePayloadWrapper(PSIBuffer&, const Section&) override;
        virtual void addOneSectionImpl(BinaryTable&, PSIBuffer&) const override;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC Stream Descriptors table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptorsTable.h"

namespace ts {
    //!
    //! Representation of a DSM-CC Stream Descriptors table.
    //!
    //! @see ISO/IEC 13818-6, 9.2.2 and 9.2.7
    //! @ingroup table
    //!
    class TSDUCKDLL DSMCCStreamDescriptorsTable : public AbstractDescriptorsTable
    {
    public:
        uint16_t& table_id_extension; //!< User-defined in the case of DSM-CC Stream Descriptors table.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] tid_ext User-defined table id extension.
        //!
        DSMCCStreamDescriptorsTable(uint8_t vers = 0, bool cur = true, uint16_t tid_ext = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DSMCCStreamDescriptorsTable(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DSMCCStreamDescriptorsTable(const DSMCCStreamDescriptorsTable& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        DSMCCStreamDescriptorsTable& operator=(const DSMCCStreamDescriptorsTable& other);

        // Inherited methods
        virtual bool isPrivate() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

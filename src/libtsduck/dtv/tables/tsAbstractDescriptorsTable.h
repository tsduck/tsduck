//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for MPEG tables containing only a list of descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Base class for MPEG tables containing only a list of descriptors (eg. CAT, TSDT).
    //! @ingroup table
    //!
    class TSDUCKDLL AbstractDescriptorsTable : public AbstractLongTable
    {
    public:
        DescriptorList descs; //!< List of descriptors.

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        //!
        //! Table_id extension.
        //! When unused (CAT, TSDT), it must be left to the default value 0xFFFF.
        //!
        uint16_t _tid_ext = 0xFFFF;

        //!
        //! Constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] tid_ext Table id extension.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        AbstractDescriptorsTable(TID tid, const UChar* xml_name, Standards standards, uint16_t tid_ext, uint8_t version, bool is_current);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AbstractDescriptorsTable(const AbstractDescriptorsTable& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractDescriptorsTable& operator=(const AbstractDescriptorsTable& other) = default;

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] table Binary table to deserialize.
        //!
        AbstractDescriptorsTable(DuckContext& duck, TID tid, const UChar* xml_name, Standards standards, const BinaryTable& table);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        AbstractDescriptorsTable() = delete;
    };
}

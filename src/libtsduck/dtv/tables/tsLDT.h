//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Linked Description Table (LDT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an ISDB Linked Description Table (LDT).
    //! @see ARIB STD-B10, Part 2, 5.2.15
    //! @ingroup table
    //!
    class TSDUCKDLL LDT : public AbstractLongTable
    {
    public:
        //!
        //! Description entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Description : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Description);
            TS_DEFAULT_ASSIGMENTS(Description);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Description(const AbstractTable* table);
        };

        //!
        //! List of descriptions, indexed by description_id.
        //!
        typedef EntryWithDescriptorsMap<uint16_t, Description> DescriptionMap;

        // LDT public members:
        uint16_t       original_service_id = 0;  //!< Original service id.
        uint16_t       transport_stream_id = 0;  //!< Transport stream id.
        uint16_t       original_network_id = 0;  //!< Original network id.
        DescriptionMap descriptions;             //!< List of descriptions.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        LDT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        LDT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        LDT(const LDT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        LDT& operator=(const LDT& other) = default;

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

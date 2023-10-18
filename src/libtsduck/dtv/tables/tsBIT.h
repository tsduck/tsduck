//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Broadcaster Information Table (BIT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an ISDB Broadcaster Information Table (BIT).
    //! @see ARIB STD-B10, Part 2, 5.2.13
    //! @ingroup table
    //!
    class TSDUCKDLL BIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a broadcaster.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Broadcaster : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Broadcaster);
            TS_DEFAULT_ASSIGMENTS(Broadcaster);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Broadcaster(const AbstractTable* table);
        };

        //!
        //! List of broadcasters, indexed by broadcaster_id.
        //!
        typedef EntryWithDescriptorsMap<uint8_t, Broadcaster> BroadcasterMap;

        // BIT public members:
        uint16_t       original_network_id = 0;           //!< Original network id.
        bool           broadcast_view_propriety = false;  //!< User indication with a unit of broadcaster name is appropriate.
        DescriptorList descs;                             //!< Top-level descriptor list.
        BroadcasterMap broadcasters;                      //!< List of broadcasters descriptions.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        BIT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        BIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        BIT(const BIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        BIT& operator=(const BIT& other) = default;

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

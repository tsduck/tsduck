//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Event Relation Table (ERT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB Event Relation Table (ERT).
    //! @see ARIB STD-B10, Part 3, 5.1.2
    //! @ingroup table
    //!
    class TSDUCKDLL ERT : public AbstractLongTable
    {
    public:
        //!
        //! Relation entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Relation : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Relation);
            TS_DEFAULT_ASSIGMENTS(Relation);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Relation(const AbstractTable* table);

            uint16_t node_id = 0;           //!< Node id.
            uint8_t  collection_mode = 0;   //!< 4 bits, collection mode.
            uint16_t parent_node_id = 0;    //!< Parent node id.
            uint8_t  reference_number = 0;  //!< Reference number
        };

        //!
        //! List of relations.
        //!
        typedef EntryWithDescriptorsList<Relation> RelationList;

        // ERT public members:
        uint16_t     event_relation_id = 0;        //!< Event relation id.
        uint16_t     information_provider_id = 0;  //!< Information provider id.
        uint8_t      relation_type = 0;            //!< 4 bits, relation type.
        RelationList relations;                    //!< List of event relations.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        ERT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        ERT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ERT(const ERT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        ERT& operator=(const ERT& other) = default;

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

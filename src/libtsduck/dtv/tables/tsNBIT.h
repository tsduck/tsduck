//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Network Board Information Table (NBIT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of an ISDB Network Board Information Table (NBIT).
    //! @see ARIB STD-B10, Part 2, 5.2.14
    //! @ingroup table
    //!
    class TSDUCKDLL NBIT : public AbstractLongTable
    {
    public:
        //!
        //! Information entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Information : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Information);
            TS_DEFAULT_ASSIGMENTS(Information);
        public:
            uint8_t information_type = 0;           //!< 4 bits, information type
            uint8_t description_body_location = 0;  //!< 2 bits, where to find the description.
            uint8_t user_defined = 0;               //!< No predefined interpretation.
            std::vector<uint16_t> key_ids {};       //!< List of key ids, depends on information_type.

            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Information(const AbstractTable* table);
        };

        //!
        //! List of informations, indexed by information_id.
        //!
        typedef EntryWithDescriptorsMap<uint16_t, Information> InformationMap;

        // NBIT public members:
        uint16_t       original_network_id = 0;  //!< Original network id.
        InformationMap informations;             //!< List of informations.

        //!
        //! Default constructor.
        //! @param [in] is_body True for an NBIT carrying actual information body, false when carrying reference to information.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        NBIT(bool is_body = true, uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        NBIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        NBIT(const NBIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        NBIT& operator=(const NBIT& other) = default;

        //!
        //! Check if this is an NBIT carrying actual information body.
        //! @return True for an NBIT carrying actual information body.
        //!
        bool isBody() const
        {
            return _table_id == TID_NBIT_BODY;
        }

        //!
        //! Check if this is an NBIT carrying reference to information body.
        //! @return True for an NBIT carrying reference to information body.
        //!
        bool isReference() const
        {
            return _table_id == TID_NBIT_REF;
        }

        //!
        //! Set the NBIT as carrying actual information body.
        //!
        void setBody()
        {
            _table_id = TID_NBIT_BODY;
        }

        //!
        //! Set the NBIT as carrying reference to information body.
        //!
        void setReference()
        {
            _table_id = TID_NBIT_REF;
        }

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual bool isValidTableId(TID) const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

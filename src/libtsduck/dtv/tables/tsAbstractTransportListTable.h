//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for tables containing a list of transport stream
//!  descriptions. Common code for BAT and NIT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Abstract base class for tables containing a list of transport stream descriptions.
    //! Common code for BAT and NIT.
    //! @ingroup table
    //!
    class TSDUCKDLL AbstractTransportListTable : public AbstractLongTable
    {
    public:
        //!
        //! Description of a transport stream.
        //!
        //! The field @c preferred_section indicates in which section a TS should be preferably serialized.
        //! When unspecified for a TS, the corresponding TS description is serialized in an arbitrary section.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Transport : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Transport);
            TS_DEFAULT_ASSIGMENTS(Transport);
        public:
            // Public members
            int preferred_section = -1;  //!< Preferred section index for serialization (-1 means no preference).

            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            explicit Transport(const AbstractTable* table);
        };

        //!
        //! List of Transport's, indexed by TransportStreamId.
        //!
        typedef EntryWithDescriptorsMap<TransportStreamId, Transport> TransportMap;


        // NIT/BAT common public members:
        DescriptorList descs;       //!< Top-level descriptor list.
        TransportMap   transports;  //!< Map of TS descriptions, key=onid/tsid, value=descriptor_list.

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AbstractTransportListTable(const AbstractTransportListTable& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractTransportListTable& operator=(const AbstractTransportListTable& other) = default;

        //!
        //! Clear preferred section in all transport.
        //!
        void clearPreferredSections();

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;

    protected:
        //!
        //! Table id extension.
        //! Interpretation of TID-extension differs between NIT and BAT.
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
        AbstractTransportListTable(TID tid, const UChar* xml_name, Standards standards, uint16_t tid_ext, uint8_t version, bool is_current);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] table Binary table to deserialize.
        //!
        AbstractTransportListTable(DuckContext& duck, TID tid, const UChar* xml_name, Standards standards, const BinaryTable& table);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;

    private:
        typedef std::set<TransportStreamId> TransportStreamIdSet;

        // Add a new section to a table being serialized, while inside transport loop.
        void addSection(BinaryTable& table, PSIBuffer& payload, bool last_section) const;

        // Select a transport stream for serialization in current section.
        // If found, set ts_id, remove the ts id from the set and return true.
        // Otherwise, return false.
        bool getNextTransport(TransportStreamIdSet& ts_set, TransportStreamId& ts_id, int section_number) const;
    };
}

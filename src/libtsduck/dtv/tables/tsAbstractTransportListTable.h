//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        public:
            // Public members
            int preferred_section;  //!< Preferred section index for serialization (-1 means no preference).

            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            explicit Transport(const AbstractTable* table);

        private:
            // Inaccessible operations.
            Transport() = delete;
            Transport(const Transport&) = delete;
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
        uint16_t _tid_ext;

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

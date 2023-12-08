//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB LDT_linkage_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB LDT_linkage_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.40
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBLDTLinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Linkage description type.
        //!
        class DescriptionType
        {
            TS_DEFAULT_COPY_MOVE(DescriptionType);
        public:
            uint16_t id = 0;            //!< The id_number of the linked descriptor.
            uint8_t  type = 0;          //!< 4 bits. The linked description type in accordance with table 6-78.
            uint8_t  user_defined = 0;  //!< The service provider can define this 8-bit field independently.
            //!
            //! Default constructor.
            //!
            DescriptionType() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            DescriptionType(PSIBuffer& buf) : DescriptionType() { deserialize(buf); }

            //!
            //! toString
            //! @returns a string depiction of the prefectures included in the target region in a bitmap form where each represented by a 1 or 0 character.
            //!
            UString toString() const;

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        // ISDBLDTLinkageDescriptor public members:
        uint16_t original_service_id = 0;               //!< The original_service_id of the linked LDT sub_table.
        uint16_t transport_stream_id = 0;               //!< The ts_id of the LDT sub_table which the linked LDT sub_table is included.
        uint16_t original_network_id = 0;               //!< The network_id of the originating delivery system in which the linked LDT sub_table is included.
        std::vector<DescriptionType> descriptions {};   //!< Linkage descriptions.

        //!
        //! Default constructor.
        //!
        ISDBLDTLinkageDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBLDTLinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}  // namespace ts

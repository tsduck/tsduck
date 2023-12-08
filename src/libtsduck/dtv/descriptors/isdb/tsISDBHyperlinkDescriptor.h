//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB hyperlink_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"


namespace ts {
    //!
    //! Representation of an ISDB hyperlink_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.29
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBHyperlinkDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service triplet.
        //!
        class ServiceTriplet
        {
            TS_DEFAULT_COPY_MOVE(ServiceTriplet);
        public:
            uint16_t original_network_id = 0;   //!< Label identifying the network_id of the originating delivery system where the linked service belongs.
            uint16_t transport_stream_id = 0;   //!< Label identifying the Transport Stream where the linked service belongs.
            uint16_t service_id = 0;            //!< Label identifying the service in the linked Transport Stream and describes the same service_id as the program_number in the corresponding program map section.
            //!
            //! Default constructor.
            //!
            ServiceTriplet() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            ServiceTriplet(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Event triplet (quadruplet in fact).
        //!
        class EventTriplet : public ServiceTriplet
        {
            TS_DEFAULT_COPY_MOVE(EventTriplet);
        public:
            uint16_t event_id = 0;      //!< The identifier number of the linked event
            //!
            //! Default constructor.
            //!
            EventTriplet() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            EventTriplet(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Module triplet.
        //!
        class ModuleTriplet : public EventTriplet
        {
            TS_DEFAULT_COPY_MOVE(ModuleTriplet);
        public:
            uint8_t  component_tag = 0; //!< Label identifying the component stream transmitting the linked carousel module.
            uint16_t module_id = 0;     //!< The identifier number of the linked carousel module.
            //!
            //! Default constructor.
            //!
            ModuleTriplet() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            ModuleTriplet(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Content triplet.
        //!
        class ContentTriplet : public ServiceTriplet
        {
            TS_DEFAULT_COPY_MOVE(ContentTriplet);
        public:
            uint16_t content_id = 0;    //!< Number to identify linked contents in the service uniformly.
            //!
            //! Default constructor.
            //!
            ContentTriplet() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            ContentTriplet(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Content module triplet.
        //!
        class ContentModuleTriplet : public ContentTriplet
        {
            TS_DEFAULT_COPY_MOVE(ContentModuleTriplet);
        public:
            uint8_t  component_tag = 0;     //!< Label identifying the component stream transmitting the linked carousel module.
            uint16_t module_id = 0;         //!< The identifier number of the linked module.
            //!
            //! Default constructor.
            //!
            ContentModuleTriplet() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            ContentModuleTriplet(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Event relation node.
        //!
        class ERTNode
        {
            TS_DEFAULT_COPY_MOVE(ERTNode);
        public:
            uint16_t information_provider_id = 0;   //!< Information provider identifier of event relation sub_table to which the linked node belongs.
            uint16_t event_relation_id = 0;         //!< Event relation identifier of event relation sub_table to which the linked destination belongs.
            uint16_t node_id = 0;                   //!< Node identifier of linked destination node.
            //!
            //! Default constructor.
            //!
            ERTNode() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            ERTNode(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Stored content.
        //!
        class StoredContent
        {
            TS_DEFAULT_COPY_MOVE(StoredContent);
        public:
            UString uri {};     //!< URI of the contents of the accumulated data service. Describing method of URI is specified in ARIB STD - B24 Part 2 Section 9.
            //!
            //! Default constructor.
            //!
            StoredContent() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            StoredContent(PSIBuffer& buf) { deserialize(buf); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };


    public:
        // ISDBHyperlinkDescriptor public members:
        uint8_t                             hyper_linkage_type = 0;     //!< linkage type in accordance with table 6-50.
        uint8_t                             link_destination_type = 0;  //!< link designation type in accordance with table 6-51.
        std::optional<ServiceTriplet>       link_to_service {};         //!< optional link to service.
        std::optional<EventTriplet>         link_to_event {};           //!< optional link to event.
        std::optional<ModuleTriplet>        link_to_module {};          //!< optional link to modules.
        std::optional<ContentTriplet>       link_to_content {};         //!< optional link to content.
        std::optional<ContentModuleTriplet> link_to_content_module {};  //!< optional link to content module.
        std::optional<ERTNode>              link_to_ert_node {};        //!< optional link to event relation node.
        std::optional<StoredContent>        link_to_stored_content {};  //!< optional link to stored content.
        ByteBlock                           private_data {};            //!< private data bytes

        //!
        //! Default constructor.
        //!
        ISDBHyperlinkDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBHyperlinkDescriptor(DuckContext& duck, const Descriptor& bin);

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
}

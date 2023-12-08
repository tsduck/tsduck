//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB component_group_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB Lcomponent_group_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.37
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBComponentGroupDescriptor: public AbstractDescriptor
    {
    public:
        //!
        //! Component group.
        //!
        class ComponentGroup
        {
            TS_DEFAULT_COPY_MOVE(ComponentGroup);
        public:
            //!
            //! CA unit.
            //!
            class CAUnit {
                TS_DEFAULT_COPY_MOVE(CAUnit);
            public:
                uint8_t   CA_unit_id = 0;    //!< 4 bits. The CA_unit_id, to which the component belongs in accordance with table 6-74.
                ByteBlock component_tags {}; //!< Component tag value belonging to the component group.

                //!
                //! Default constructor.
                //!
                CAUnit() = default;
                //!
                //! Read-in constructor.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                CAUnit(PSIBuffer& buf) : CAUnit() { deserialize(buf); }

                //! @cond nodoxygen
                void clear();
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&, size_t);
                //! @endcond
            };

            uint8_t                component_group_id = 0; //!< The component group identifier in accordance with table 6-73.
            std::vector<CAUnit>    CA_units {};            //!< All CA units.
            std::optional<uint8_t> total_bit_rate {};      //!< The component tag value belonging to the component group.
            UString                explanation {};         //!< Explanation of component group. For character information coding, see Annex A.

            //!
            //! Default constructor.
            //!
            ComponentGroup() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //! @param [in] total_bit_rate_flag Indicates the description status of the total bit rate in the component group in the event.When this bit is "0", the total bit rate
            //! field in the component group does not exist in the corresponding descriptor. When this bit is "1", the total bit rate field in the
            //! component group exists in the corresponding descriptor.
            //!
            ComponentGroup(PSIBuffer& buf, bool total_bit_rate_flag) :
                ComponentGroup() { deserialize(buf, total_bit_rate_flag); }

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&, bool) const;
            void deserialize(PSIBuffer&, bool);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&, bool, size_t);
            //! @endcond
        };

        // ISDBComponentGroupDescriptor public members:
        uint8_t                     component_group_type = 0;  //!< 3 bits. Group type of the component in accordance with table 6 - 72.
        std::vector<ComponentGroup> components {};             //!< All components.

        //!
        //! Default constructor.
        //!
        ISDBComponentGroupDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBComponentGroupDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! matching_total_bit_rate
        //! @return True if the total_bit_rate value is set or unset for all #components.
        //!
        bool matching_total_bit_rate();

        //!
        //! total_bit_rate_flag
        //! @return true if all of the #components have a total_bit_rate value
        //!
        bool total_bit_rate_flag() const;

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

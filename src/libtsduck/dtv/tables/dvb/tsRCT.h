//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Related Content Table (RCT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of a Related Content Table (RCT)
    //! @see ETSI TS 102 323, 10.4.2
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL RCT : public AbstractLongTable
    {
    public:
        //!
        //! Description of DVB binary locator.
        //! @see ETSI TS 102 323, 7.3.2.3.3
        //!
        class TSDUCKDLL DVBBinaryLocator
        {
            TS_DEFAULT_COPY_MOVE(DVBBinaryLocator);
        public:
            uint8_t  identifier_type = 0;                //!< 2 bits.
            bool     scheduled_time_reliability = false; //!< 1 bit.
            bool     inline_service = false;             //!< 1 bit.
            uint16_t start_date = 0;                     //!< 9 bits, number of days from the beginning of the year indicated by year_offset field in enclosing structure.
            uint16_t dvb_service_triplet_id = 0;         //!< 10 bits, when inline_service == false.
            uint16_t transport_stream_id = 0;            //!< 16 bits, when inline_service == true.
            uint16_t original_network_id = 0;            //!< 16 bits, when inline_service == true.
            uint16_t service_id = 0;                     //!< 16 bits, when inline_service == true.
            uint16_t start_time = 0;                     //!< 16 bits, number of 2-second periods since midnight.
            uint16_t duration = 0;                       //!< 16 bits, count of 2-second periods.
            uint16_t event_id = 0;                       //!< 16 bits, when identifier_type == 1.
            uint16_t TVA_id = 0;                         //!< 16 bits, when identifier_type == 2 or 3.
            uint8_t  component_tag = 0;                  //!< 8 bits, when identifier_type == 3.
            uint8_t  early_start_window = 0;             //!< 3 bits, when identifier_type == 0 && scheduled_time_reliability == true.
            uint8_t  late_end_window = 0;                //!< 5 bits, when identifier_type == 0 && scheduled_time_reliability == true.

            //! @cond nodoxygen
            // Delegated methods.
            DVBBinaryLocator() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* parent) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t year_offset);
            //! @endcond
        };

        //!
        //! Promotional text.
        //!
        class TSDUCKDLL PromotionalText
        {
            TS_DEFAULT_COPY_MOVE(PromotionalText);
        public:
            UString language_code {};  //!< ISO-639 language code, 3 characters.
            UString text {};           //!< Text info.

            //! @cond nodoxygen
            // Delegated methods.
            PromotionalText() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* parent) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! Description of link information.
        //! @see ETSI TS 102 323, 10.4.3
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Link : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Link);
            TS_DEFAULT_ASSIGMENTS(Link);
        public:
            uint8_t  link_type = 0;                             //!< 4 bits.
            uint8_t  how_related_classification_scheme_id = 0;  //!< 6 bits.
            uint16_t term_id = 0;                               //!< 12 bits.
            uint8_t  group_id = 0;                              //!< 4 bits.
            uint8_t  precedence = 0;                            //!< 4 bits.
            UString  media_uri {};                              //!< when link_type == 0 or 2.
            bool     default_icon_flag = false;                 //!< 1 bit.
            uint8_t  icon_id = 0;                               //!< 3 bits.
            DVBBinaryLocator dvb_binary_locator {};             //!< when link_type == 1 or 2.
            std::list<PromotionalText> promotional_texts {};    //!< any number.

            //! @cond nodoxygen
            // Delegated methods.
            Link(const AbstractTable* table);
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* parent) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, const ts::Section& section, DescriptorContext& context, PSIBuffer& buf, const UString& margin, uint16_t year_offset);
            //! @endcond
        };

        //!
        //! List of link information entries.
        //!
        using LinkList = AttachedEntryList<Link>;

        // RCT public members:
        uint16_t       service_id = 0;  //!< Service id.
        uint16_t       year_offset = 0; //!< Year relative to which date values in this structure shall be calculated
        LinkList       links {this};    //!< List of link information.
        DescriptorList descs {this};    //!< Top-level descriptor list.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        RCT(uint8_t vers = 0, bool cur = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        RCT(const RCT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        RCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        RCT& operator=(const RCT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        virtual DescriptorList* topLevelDescriptorList() override;
        virtual const DescriptorList* topLevelDescriptorList() const override;
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

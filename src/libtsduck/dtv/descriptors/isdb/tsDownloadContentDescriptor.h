//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB download_content_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB download_content_descriptor.
    //! @see ARIB STD-B21, 12.2.1.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DownloadContentDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! ISDB download content subdescriptor.
        //!
        class TSDUCKDLL ContentSubdescriptor
        {
        public:
            uint8_t   type = 0;                   //!< Subdescriptor type.
            ByteBlock additional_information {};  //!< Subdescriptor data.

            //! @cond nodoxygen
            // Delegated methods.
            ContentSubdescriptor() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content descriptor.
        //!
        class TSDUCKDLL ContentDescriptor
        {
        public:
            uint8_t  descriptor_type = 0;              //!< See ARIB STD-B21, 12.2.1.1.
            uint8_t  specifier_type = 0;               //!< See ARIB STD-B21, 12.2.1.1.
            uint32_t specifier_data = 0;               //!< 24 bits. See ARIB STD-B21, 12.2.1.1.
            uint16_t model = 0;                        //!< See ARIB STD-B21, 12.2.1.1.
            uint16_t version = 0;                      //!< See ARIB STD-B21, 12.2.1.1.
            std::list<ContentSubdescriptor> subdescs;  //!< List of subdescriptors.

            //! @cond nodoxygen
            // Delegated methods.
            ContentDescriptor() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content compatibility descriptor.
        //!
        class TSDUCKDLL CompatibilityDescriptor : public std::list<ContentDescriptor>
        {
        public:
            //! @cond nodoxygen
            // Delegated methods.
            CompatibilityDescriptor() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* parent);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content module.
        //!
        class TSDUCKDLL Module
        {
        public:
            uint16_t  module_id = 0;    //!< See ARIB STD-B21, 12.2.1.1.
            uint32_t  module_size = 0;  //!< See ARIB STD-B21, 12.2.1.1.
            ByteBlock module_info {};   //!< See ARIB STD-B21, 12.2.1.1.

            //! @cond nodoxygen
            // Delegated methods.
            Module() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! List of ISDB download content modules.
        //!
        class TSDUCKDLL ModuleInfo : public std::list<Module>
        {
        public:
            //! @cond nodoxygen
            // Delegated methods.
            ModuleInfo() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* parent);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download text info.
        //!
        class TSDUCKDLL TextInfo
        {
        public:
            UString ISO_639_language_code {};  //!< ISO-639 language code, 3 characters.
            UString text {};                   //!< Text info.

            //! @cond nodoxygen
            // Delegated methods.
            TextInfo() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        // DownloadContentDescriptor public members:
        bool     reboot = false;          //!< See ARIB STD-B21, 12.2.1.1.
        bool     add_on = false;          //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t component_size = 0;      //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t download_id = 0;         //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t time_out_value_DII = 0;  //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t leak_rate = 0;           //!< 22 bits. See ARIB STD-B21, 12.2.1.1.
        uint8_t  component_tag = 0;       //!< See ARIB STD-B21, 12.2.1.1.
        CompatibilityDescriptor compatibility_descriptor {}; //!< Compatibility descriptor.
        ModuleInfo              module_info {};              //!< List of modules.
        ByteBlock               private_data {};             //!< Private data.
        std::optional<TextInfo> text_info {};                //!< Optional text info.

        //!
        //! Default constructor.
        //!
        DownloadContentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DownloadContentDescriptor(DuckContext& duck, const Descriptor& bin);

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

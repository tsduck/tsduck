//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for ISDB download content descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base class for ISDB download content descriptors.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AbstractDownloadContentDescriptor : public AbstractDescriptor
    {
        TS_RULE_OF_FIVE(AbstractDownloadContentDescriptor, override);
    public:
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

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractDownloadContentDescriptor(EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr) :
            AbstractDescriptor(edid, xml_name, xml_legacy_name)
        {
        }
    };
}

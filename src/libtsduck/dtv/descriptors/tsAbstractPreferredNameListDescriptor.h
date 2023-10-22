//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract representation of a preferred_name_list_descriptor
//!  for different private data specifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Abstract representation of a preferred_name_list_descriptor for different private data specifiers.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AbstractPreferredNameListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Maximum number of preferred names per descriptor.
        //! Defined in EACEM TR-030 section 9.2.11.2 and in the D-Book 7 Part A section 8.5.3.7.
        //!
        static constexpr size_t MAX_PREFERRED_NAMES = 5;

        //!
        //! For each language, there is a map of service names per 8-bit name_id.
        //!
        typedef std::map<uint8_t, UString> NameByIdMap;

        //!
        //! There is a map of service name sets per language.
        //!
        typedef std::map<UString, NameByIdMap> LanguageMap;

        // AbstractPreferredNameListDescriptor public members:
        LanguageMap entries {};  //!< Map of language entries.

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        //!
        //! Default constructor.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractPreferredNameListDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds, const UChar* xml_legacy_name = nullptr);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractPreferredNameListDescriptor(DuckContext& duck, const Descriptor& bin, DID tag, const UChar* xml_name, Standards standards, PDS pds, const UChar* xml_legacy_name = nullptr);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        AbstractPreferredNameListDescriptor() = delete;
    };
}

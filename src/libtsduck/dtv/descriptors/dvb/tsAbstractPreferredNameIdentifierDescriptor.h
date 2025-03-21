//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract representation of a preferred_name_identifier_descriptor
//!  for different private data specifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Abstract representation of a preferred_name_identifier_descriptor for different private data specifiers.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AbstractPreferredNameIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // AbstractPreferredNameIdentifierDescriptor public members:
        uint8_t name_id = 0;  //!< Service name id from an EacemPreferredNameListDescriptor.

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        //!
        //! Default constructor.
        //! @param [in] name_id Service name id from an EacemPreferredNameListDescriptor.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractPreferredNameIdentifierDescriptor(uint8_t name_id, EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& bin, EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        AbstractPreferredNameIdentifierDescriptor() = delete;
    };
}

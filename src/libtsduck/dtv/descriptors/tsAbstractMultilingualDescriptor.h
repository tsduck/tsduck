//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for DVB descriptors with a multilingual name.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Abstract base class for DVB descriptors with a multilingual name.
    //! Subclasses may have a "prolog" between the descriptor header and
    //! the multilingual names loop.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AbstractMultilingualDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Language entry.
        //!
        struct TSDUCKDLL Entry
        {
            UString language {};  //!< ISO-639 language code, 3 characters.
            UString name {};      //!< Name in this language.
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        // Multiligual descriptor public members:
        EntryList entries {};  //!< List of language entries.

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_attribute XML attribute name for the "name" fields.
        //!
        AbstractMultilingualDescriptor(DID tag, const UChar* xml_name, const UChar* xml_attribute);

        // Use default assignment but declare it to make sure the compiler knows
        // that we have understood the consequences of a pointer member.
        //! @cond nodoxygen
        AbstractMultilingualDescriptor(const AbstractMultilingualDescriptor&) = default;
        AbstractMultilingualDescriptor& operator=(const AbstractMultilingualDescriptor&) = default;
        //! @endcond

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        const UChar* _xml_attribute = nullptr;

        // Inaccessible operations.
        AbstractMultilingualDescriptor() = delete;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a multilingual_service_name_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a multilingual_service_name_descriptor.
    //! @see ETSI EN 300 468, 6.2.25.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MultilingualServiceNameDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Language entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString language {};               //!< ISO-639 language code, 3 characters.
            UString service_provider_name {};  //!< Service provider name in this language.
            UString service_name {};           //!< Service name in this language.

            //!
            //! Default constructor.
            //! @param [in] lang ISO-639 language code, 3 characters.
            //! @param [in] prov Service provider name for this language.
            //! @param [in] name Service name for this language.
            //!
            Entry(const UString& lang = UString(), const UString& prov = UString(), const UString& name = UString());
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        // MultilingualServiceNameDescriptor public members:
        EntryList entries {};  //!< List of language entries.

        //!
        //! Default constructor.
        //!
        MultilingualServiceNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MultilingualServiceNameDescriptor(DuckContext& duck, const Descriptor& bin);

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

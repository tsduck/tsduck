//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an external_application_authorization_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsApplicationIdentifier.h"

namespace ts {
    //!
    //! Representation of an external_application_authorization_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.5.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ExternalApplicationAuthorizationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Application entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            ApplicationIdentifier application_identifier {};  //!< Application identifier.
            uint8_t               application_priority = 0;   //!< Application priority.

            //!
            //! Default constructor.
            //! @param [in] org_id Organization identifier.
            //! @param [in] app_id Application identifier.
            //! @param [in] prio Application priority.
            //!
            Entry(uint32_t org_id = 0, uint16_t app_id = 0, uint8_t prio = 0) :
                application_identifier(org_id, app_id),
                application_priority(prio)
            {
            }
        };

        //!
        //! List of application entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 36;

        // ExternalApplicationAuthorizationDescriptor public members:
        EntryList entries {};  //!< The list of application entries.

        //!
        //! Default constructor.
        //!
        ExternalApplicationAuthorizationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ExternalApplicationAuthorizationDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an application_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an application_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.5.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Layout of an application profile.
        //!
        struct Profile
        {
            Profile();                     //!< Default constructor.
            uint16_t application_profile;  //!< Application profile code.
            uint8_t  version_major;        //!< Major version number.
            uint8_t  version_minor;        //!< Minor version number.
            uint8_t  version_micro;        //!< Mico version number.
        };

        //!
        //! List of application profiles.
        //!
        typedef std::list<Profile> ProfileList;

        // ApplicationDescriptor public members:
        ProfileList profiles;                   //!< List of application profiles.
        bool        service_bound;              //!< Application is bound to current service.
        uint8_t     visibility;                 //!< Visibility code, 2 bits.
        uint8_t     application_priority;       //!< Application priority.
        ByteBlock   transport_protocol_labels;  //!< One byte per transport protocol label.

        //!
        //! Default constructor.
        //!
        ApplicationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationDescriptor(DuckContext& duck, const Descriptor& bin);

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

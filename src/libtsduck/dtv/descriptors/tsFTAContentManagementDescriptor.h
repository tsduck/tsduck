//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a FTA_content_management_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a FTA_content_management_descriptor.
    //! @see ETSI EN 300 468, 6.2.18.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL FTAContentManagementDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool    user_defined = false;                     //!< Reserved to user.
        bool    do_not_scramble = false;                  //!< Do not scramble.
        uint8_t control_remote_access_over_internet = 0;  //!< 2 bits, access control over Internet.
        bool    do_not_apply_revocation = false;          //!< Do not apply revocation.

        //!
        //! Default constructor.
        //!
        FTAContentManagementDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        FTAContentManagementDescriptor(DuckContext& duck, const Descriptor& bin);

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

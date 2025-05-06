//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB network_download_content_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDownloadContentDescriptor.h"
#include "tsDSMCCCompatibilityDescriptor.h"
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! Representation of an ISDB network_download_content_descriptor.
    //! @see ARIB STD-B21, 12.2.1.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL NetworkDownloadContentDescriptor : public AbstractDownloadContentDescriptor
    {
    public:
        // NetworkDownloadContentDescriptor public members:
        bool                           reboot = false;              //!< See ARIB STD-B21, 12.2.1.1.
        bool                           add_on = false;              //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                       component_size = 0;          //!< See ARIB STD-B21, 12.2.1.1.
        uint8_t                        session_protocol_number = 0; //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                       session_id = 0;              //!< See ARIB STD-B21, 12.2.1.1.
        uint8_t                        retry = 0;                   //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                       connect_timer = 0;           //!< 24 bits. See ARIB STD-B21, 12.2.1.1.
        std::optional<IPSocketAddress> ip {};                       //!< Exactly one of ip or url must be set.
        std::optional<UString>         url {};                      //!< Exactly one of ip or url must be set.
        DSMCCCompatibilityDescriptor   compatibility_descriptor {}; //!< Compatibility descriptor.
        ByteBlock                      private_data {};             //!< Private data.
        std::optional<TextInfo>        text_info {};                //!< Optional text info.

        //!
        //! Default constructor.
        //!
        NetworkDownloadContentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NetworkDownloadContentDescriptor(DuckContext& duck, const Descriptor& bin);

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

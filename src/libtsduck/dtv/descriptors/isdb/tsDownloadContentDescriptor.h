//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB download_content_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDownloadContentDescriptor.h"
#include "tsDSMCCCompatibilityDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB download_content_descriptor.
    //! @see ARIB STD-B21, 12.2.1.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DownloadContentDescriptor : public AbstractDownloadContentDescriptor
    {
    public:
        // DownloadContentDescriptor public members:
        bool                        reboot = false;              //!< See ARIB STD-B21, 12.2.1.1.
        bool                         add_on = false;              //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                     component_size = 0;          //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                     download_id = 0;             //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                     time_out_value_DII = 0;      //!< See ARIB STD-B21, 12.2.1.1.
        uint32_t                     leak_rate = 0;               //!< 22 bits. See ARIB STD-B21, 12.2.1.1.
        uint8_t                      component_tag = 0;           //!< See ARIB STD-B21, 12.2.1.1.
        DSMCCCompatibilityDescriptor compatibility_descriptor {}; //!< Compatibility descriptor.
        std::list<Module>            module_info {};              //!< List of modules.
        ByteBlock                    private_data {};             //!< Private data.
        std::optional<TextInfo>      text_info {};                //!< Optional text info.

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

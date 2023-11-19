//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB AC-4_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB AC-4_descriptor.
    //! @see ETSI EN 300 468, D.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DVBAC4Descriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        std::optional<bool>    ac4_dialog_enhancement_enabled {}; //!< See ETSI EN 300 468, D.7.
        std::optional<uint8_t> ac4_channel_mode {};               //!< 2 bits, see ETSI EN 300 468, D.7.
        ByteBlock              ac4_dsi_toc {};                    //!< See ETSI EN 300 468, D.7.
        ByteBlock              additional_info {};                //!< See ETSI EN 300 468, D.7.

        //!
        //! Default constructor.
        //!
        DVBAC4Descriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBAC4Descriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };

    //!
    //! Legacy name for a DVB AC-4_descriptor.
    //!
    typedef DVBAC4Descriptor AC4Descriptor;
}

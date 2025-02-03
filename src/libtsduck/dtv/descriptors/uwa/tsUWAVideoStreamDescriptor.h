//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an CUVV_video_stream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an CUVV_video_stream_descriptor.
    //! @see T/UWA 005-2.1.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL UWAVideoStreamDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint32_t  cuvv_tag = 0;                        //!< See T/UWA 005-2.1.
        uint16_t  cuva_version_map = 0;                //!< See T/UWA 005-2.1.
        uint16_t  terminal_provide_code = 0;           //!< See T/UWA 005-2.1.
        int       terminal_provide_oriented_code = 0;  //!< See T/UWA 005-2.1.

        //!
        //! Default constructor.
        //!
        UWAVideoStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        UWAVideoStreamDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Thread-safe init-safe static data patterns.
        static const Names& VersionNumbers();
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DRM_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DRM_descriptor.
    //! @see ITU J.1041 clause 7.2.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DTMBDRMDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t video_format = 0;                       //!< 4 bits. See ITU J.1041 table 7-4
        uint8_t video_encryption_method = 0;            //!< 4 bits. See ITU J.1041 table 7-5
        uint8_t audio_format = 0;                       //!< 4 bits. See ITU J.1041
        uint8_t audio_encryption_method = 0;            //!< 4 bits. See ITU J.1041

        ByteBlock DRM_data_bytes {};                    //!< relevant information for obtaining the license .

        //!
        //! Default constructor.
        //!
        DTMBDRMDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTMBDRMDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MPEG4_audio_extension_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a MPEG4_audio_extension_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.72.
    //! @ingroup descriptor
    //!
    //! This is a limited implementation. The audioSpecificConfig() is represented
    //! as a binary blob. A future version may chose to describe the internal structure
    //! of this data block.
    //!
    class TSDUCKDLL MPEG4AudioExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // MPEG4AudioExtensionDescriptor public members:
        ByteBlock audioProfileLevelIndication {};   //!< Up to 15 audio profile level 8-bit values.
        ByteBlock audioSpecificConfig {};           //!< Binary structure as defined in 1.6.2.1 in ISO/IEC 14496-3.

        static constexpr size_t MAX_PROFILES = 15;  //!< Maximum number of audio profile level 8-bit values.

        //!
        //! Default constructor.
        //!
        MPEG4AudioExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEG4AudioExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

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

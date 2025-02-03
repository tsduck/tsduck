//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEGH_3D_audio_config_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEGH_3D_audio_config_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.108.
    //! @ingroup libtsduck descriptor
    //!
    //! This is a limited implementation. The mpegh3daConfig() is represented
    //! as a binary blob. A future version may chose to describe the internal structure
    //! of this data block.
    //!
    class TSDUCKDLL MPEGH3DAudioConfigDescriptor : public AbstractDescriptor
    {
    public:
        // MPEGH3DAudioConfigDescriptor public members:
        ByteBlock mpegh3daConfig {};  //!< Binary structure as defined in 5.2.2.1 in ISO/IEC 23008-3

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioConfigDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioConfigDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined MPEGH_3D_audio_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined MPEGH_3D_audio_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.106.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioDescriptor : public AbstractDescriptor
    {
    public:
        // MPEGH3DAudioDescriptor public members:
        uint8_t   mpegh_3da_profile_level_indication = 0;  //!< MPEGH 3D-audio profile per ISO/IEC 23008-03.
        bool      interactivity_enabled = false;           //!< 3D audio stream contains elements which enables user interactivity.
        uint8_t   reference_channel_layout = 0;            //!< 6 bits, see "ChannelConfiguration" in ISO/IEC 23001-8 -> ISO//IEC 23091-3.
        ByteBlock CompatibleSetIndication {};              //!< array of 8 bit values according to ISO/IEC 23008-3
        ByteBlock reserved {};                             //!< Reserved data.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioDescriptor(DuckContext& duck, const Descriptor& bin);

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
}

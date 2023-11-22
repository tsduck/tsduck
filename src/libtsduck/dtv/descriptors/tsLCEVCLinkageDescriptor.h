//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an LCEVC_linkage_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an LCEVC_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1 (Amd.1) 2.6.137, ITU-T Rec. H.222.0.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LCEVCLinkageDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:


        ByteBlock lcevc_stream_tags {};  //!< array of 8 bit values.

        //!
        //! Default constructor.
        //!
        LCEVCLinkageDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LCEVCLinkageDescriptor(DuckContext& duck, const Descriptor& bin);

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

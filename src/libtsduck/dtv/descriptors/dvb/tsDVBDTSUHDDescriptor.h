//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB DTS-UHD_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB DTS-UHD descriptor.
    //! @see ETSI EN 300 468, annex G.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DVBDTSUHDDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   DecoderProfileCode = 0;  //!< 6 bits
        uint8_t   FrameDurationCode = 0;   //!< 2 bits
        uint8_t   MaxPayloadCode = 0;      //!< 3 bits
        uint8_t   StreamIndex = 0;         //!< 3 bits
        ByteBlock codec_selector {};       //!< Selector bytes

        //!
        //! Default constructor.
        //!
        DVBDTSUHDDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBDTSUHDDescriptor(DuckContext& duck, const Descriptor& bin);

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

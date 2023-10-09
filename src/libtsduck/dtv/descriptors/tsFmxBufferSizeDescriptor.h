//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an FMXBufferSize_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MuxCode descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.50.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL FmxBufferSizeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! One FlexMux description.
        //!
        //! Details about the FlexMuxBufferDescriptor() are no longer provided in ISO/IEC 14496-1
        //! as indicated by ISO/IEC 13818-1.
        //!
        //! This syntax can be found in section 7.2 of "The MPEG-4 Book" by Fernando Pereira and
        //! Touradj Ebrahimi. IMSC Press 2002. ISBN 0130616214
        //!
        class TSDUCKDLL FlexMuxBufferDescriptor_type {
        public:
            uint8_t     flexMuxChnnel;       //!< 8 bits
            uint32_t    FB_BufferSize;       //!< 24 bits

            FlexMuxBufferDescriptor_type();     //!< Constructor
        };

        // Public members:
        FlexMuxBufferDescriptor_type              DefaultFlexMuxBufferDescriptor;  //!< Default FlexMux.
        std::vector<FlexMuxBufferDescriptor_type> FlexMuxBufferDescriptor;         //!< Other FlexMux.

        //!
        //! Default constructor.
        //!
        FmxBufferSizeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        FmxBufferSizeDescriptor(DuckContext& duck, const Descriptor& bin);

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

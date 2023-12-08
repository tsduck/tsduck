//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an B4MuxBufferSize_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an M4 Mux Buffer Size descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.50.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL M4MuxBufferSizeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! M4Mux description.
        //!
        //! See clasue 7.4.2.4 of ISO/IEC 14496-1:2010
        //!
        class TSDUCKDLL M4MuxBufferDescriptor_type {
        public:
            uint8_t  m4MuxChnnel = 0;    //!< 8 bits
            uint32_t FB_BufferSize = 0;  //!< 24 bits

            M4MuxBufferDescriptor_type() = default;  //!< Constructor
        };

        // Public members:
        M4MuxBufferDescriptor_type               DefaultM4MuxBufferDescriptor {};  //!< Default M4Mux.
        std::vector<M4MuxBufferDescriptor_type>  M4MuxBufferDescriptor {};         //!< Other M4Mux.

        //!
        //! Default constructor.
        //!
        M4MuxBufferSizeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        M4MuxBufferSizeDescriptor(DuckContext& duck, const Descriptor& bin);

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

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC AC-3_audio_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ATSC AC-3_audio_stream_descriptor.
    //! @see ATSC A/52, A.4.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ATSCAC3AudioStreamDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   sample_rate_code;  //!< 3 bits, see A/52, A.4.3.
        uint8_t   bsid;              //!< 5 bits, see A/52, A.4.3.
        uint8_t   bit_rate_code;     //!< 6 bits, see A/52, A.4.3.
        uint8_t   surround_mode;     //!< 2 bits, see A/52, A.4.3.
        uint8_t   bsmod;             //!< 3 bits, see A/52, A.4.3.
        uint8_t   num_channels;      //!< 4 bits, see A/52, A.4.3.
        bool      full_svc;          //!< See A/52, A.4.3.
        uint8_t   mainid;            //!< 3 bits, if bsmod < 2, see A/52, A.4.3.
        uint8_t   priority;          //!< 2 bits, if bsmod < 2, see A/52, A.4.3.
        uint8_t   asvcflags;         //!< 8 bits, if bsmod >= 2, see A/52, A.4.3.
        UString   text;              //!< See A/52, A.4.3.
        UString   language;          //!< 3 chars, optional, see A/52, A.4.3.
        UString   language_2;        //!< 3 chars, optional, see A/52, A.4.3.
        ByteBlock additional_info;   //!< See A/52, A.4.3.

        //!
        //! Default constructor.
        //!
        ATSCAC3AudioStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCAC3AudioStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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

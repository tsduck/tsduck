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
//!  Representation of an ATSC E-AC-3_audio_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an ATSC E-AC-3_audio_descriptor.
    //! @see ATSC A/52, G.3.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ATSCEAC3AudioDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool              mixinfoexists;       //!< See A/52, G.3.5.
        bool              full_service;        //!< See A/52, G.3.5.
        uint8_t           audio_service_type;  //!< 3 bits, see A/52, G.3.5.
        uint8_t           number_of_channels;  //!< 3 bits, see A/52, G.3.5.
        Variable<uint8_t> bsid;                //!< 5 bits, see A/52, G.3.5.
        Variable<uint8_t> priority;            //!< 2 bits, see A/52, G.3.5.
        Variable<uint8_t> mainid;              //!< 3 bits, see A/52, G.3.5.
        Variable<uint8_t> asvc;                //!< See A/52, G.3.5.
        Variable<uint8_t> substream1;          //!< See A/52, G.3.5.
        Variable<uint8_t> substream2;          //!< See A/52, G.3.5.
        Variable<uint8_t> substream3;          //!< See A/52, G.3.5.
        UString           language;            //!< 3 chars, see A/52, G.3.5.
        UString           language_2;          //!< 3 chars, see A/52, G.3.5.
        UString           substream1_lang;     //!< 3 chars, see A/52, G.3.5.
        UString           substream2_lang;     //!< 3 chars, see A/52, G.3.5.
        UString           substream3_lang;     //!< 3 chars, see A/52, G.3.5.
        ByteBlock         additional_info;     //!< See A/52, G.3.5.

        //!
        //! Default constructor.
        //!
        ATSCEAC3AudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCEAC3AudioDescriptor(DuckContext& duck, const Descriptor& bin);

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

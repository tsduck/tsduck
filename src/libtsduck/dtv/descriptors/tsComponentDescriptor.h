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
//!  Representation of a component_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of a component_descriptor.
    //! @see ETSI EN 300 468, 6.2.8.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ComponentDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t stream_content_ext;  //!< 4 bits, see ETSI EN 300 468, 6.2.8.
        uint8_t stream_content;      //!< 4 bits, see ETSI EN 300 468, 6.2.8.
        uint8_t component_type;      //!< See ETSI EN 300 468, 6.2.8.
        uint8_t component_tag;       //!< See ETSI EN 300 468, 6.2.8.
        UString language_code;       //!< 3 chars, see ETSI EN 300 468, 6.2.8.
        UString text;                //!< See ETSI EN 300 468, 6.2.8.

        //!
        //! Default constructor.
        //!
        ComponentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ComponentDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

        //!
        //! Name of a Component Type.
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] stream_content Stream content (4 bits).
        //! @param [in] stream_content_ext Stream content extension (4 bits). Ignored for @a stream_content in the range 1..8.
        //! @param [in] component_type Component type.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @return The corresponding name.
        //!
        static UString ComponentTypeName(const DuckContext& duck, uint8_t stream_content, uint8_t stream_content_ext, uint8_t component_type, NamesFlags flags = NamesFlags::NAME, size_t bits = 16);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

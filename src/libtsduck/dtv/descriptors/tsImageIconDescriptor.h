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
//!  Representation of a image_icon_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a image_icon_descriptor
    //! @see ETSI EN 300 468, 6.4.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ImageIconDescriptor : public AbstractDescriptor
    {
    public:
        // ImageIconDescriptor public members:
        uint8_t   descriptor_number;        //!< 4 bits, index of this descriptor for this icon.
        uint8_t   last_descriptor_number;   //!< 4 bits, index of last descriptor for this icon.
        uint8_t   icon_id;                  //!< 3 bits, icon id in this descriptor loop.
        uint8_t   icon_transport_mode;      //!< 2 bits, when descriptor_number == 0.
        bool      has_position;             //!< A screen position is specified, when descriptor_number == 0.
        uint8_t   coordinate_system;        //!< 3 bits, when descriptor_number == 0 and has_position == true.
        uint16_t  icon_horizontal_origin;   //!< 12 bits, when descriptor_number == 0 and has_position == true.
        uint16_t  icon_vertical_origin;     //!< 12 bits, when descriptor_number == 0 and has_position == true.
        UString   icon_type;                //!< Icon MIME type, when descriptor_number == 0.
        UString   url;                      //!< Icon URL, when descriptor_number == 0 and icon_transport_mode == 1.
        ByteBlock icon_data;                //!< Icon data bytes, when descriptor_number > 0 or icon_transport_mode == 0.

        //!
        //! Default constructor.
        //!
        ImageIconDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ImageIconDescriptor(DuckContext& duck, const Descriptor& bin);

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

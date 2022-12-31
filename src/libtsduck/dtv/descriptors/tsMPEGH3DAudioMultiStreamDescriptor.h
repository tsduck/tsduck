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
//!  Representation of an MPEG-defined MPEGH_3D_audio_multi_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined MPEGH_3D_audio_multi_stream_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.114.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioMultiStreamDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of an mae_group (as described in ISO/IEC 23008-3, section 15).
        //!
        class TSDUCKDLL Group
        {
        public:
            Group();                      //!< Contructor.
            uint8_t mae_group_id;         //!< 7 bits.
            bool    is_in_main_stream;    //!< Audio data in this group is present in the main stream.
            bool    is_in_ts;             //!< When is_in_main_stream == false.
            uint8_t auxiliary_stream_id;  //!< 7 bits. When is_in_main_stream == false.

        };

        //!
        //! A list of mae_group (ISO/IEC 23008-3).
        //!
        typedef std::list<Group> GroupList;

        // MPEGH3DAudioMultiStreamDescriptor public members:
        bool      this_is_main_stream;    //!< The stream is a main stream, not an auxiliary stream.
        uint8_t   this_stream_id;         //!< 7 bits.
        uint8_t   num_auxiliary_streams;  //!< 7 bits. When this_is_main_stream == true.
        GroupList mae_groups;             //!< When this_is_main_stream == true.
        ByteBlock reserved;               //!< Reserved data.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioMultiStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioMultiStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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

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
//!  Representation of an SCTE 18 EAS_audio_file_descriptor
//!  (specific to a Cable Emergency Alert Table).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SCTE 18 EAS_audio_file_descriptor (specific to a Cable Emergency Alert Table).
    //!
    //! This descriptor cannot be present in other tables than a Cable Emergency Alert Table).
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 18, 5.1.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EASAudioFileDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Audio source entry.
        //!
        class TSDUCKDLL Entry
        {
        public:
            Entry();                  //!< Constructor.
            UString  file_name;       //!< Audio file name.
            uint8_t  audio_format;    //!< 7 bits, audio format.
            uint8_t  audio_source;    //!< 8 bits, audio source.
            uint16_t program_number;  //!< Program number, aka service id (audio_source == 0x01 or 0x02).
            uint32_t carousel_id;     //!< Carousel id (audio_source == 0x01).
            uint32_t download_id;     //!< Download id (audio_source == 0x02).
            uint32_t module_id;       //!< Module id (audio_source == 0x02).
            uint16_t application_id;  //!< Application id (audio_source == 0x01 or 0x02).
        };

        //!
        //! List of exception entries.
        //!
        typedef std::list<Entry> EntryList;

        // EASAudioFileDescriptor public members:
        EntryList entries;  //!< The list of audio source entries.

        //!
        //! Default constructor.
        //!
        EASAudioFileDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EASAudioFileDescriptor(DuckContext& duck, const Descriptor& bin);

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

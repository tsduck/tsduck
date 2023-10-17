//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            Entry() = default;            //!< Constructor.
            UString  file_name {};        //!< Audio file name.
            uint8_t  audio_format = 0;    //!< 7 bits, audio format.
            uint8_t  audio_source = 0;    //!< 8 bits, audio source.
            uint16_t program_number = 0;  //!< Program number, aka service id (audio_source == 0x01 or 0x02).
            uint32_t carousel_id = 0;     //!< Carousel id (audio_source == 0x01).
            uint32_t download_id = 0;     //!< Download id (audio_source == 0x02).
            uint32_t module_id = 0;       //!< Module id (audio_source == 0x02).
            uint16_t application_id = 0;  //!< Application id (audio_source == 0x01 or 0x02).
        };

        //!
        //! List of exception entries.
        //!
        typedef std::list<Entry> EntryList;

        // EASAudioFileDescriptor public members:
        EntryList entries {};  //!< The list of audio source entries.

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

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
//!  Representation of a Program Map Table (PMT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsCodecType.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of a Program Map Table (PMT).
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.8
    //! @ingroup table
    //!
    class TSDUCKDLL PMT : public AbstractLongTable
    {
    public:
        //!
        //! Description of an elementary stream.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Stream : public EntryWithDescriptors
        {
        public:
            uint8_t stream_type;  //!< Stream type, one of ST_* (eg ts::ST_MPEG2_VIDEO).

            //!
            //! Constructor.
            //! @param [in] table Parent PMT.
            //! @param [in] type Stream type.
            //!
            explicit Stream(const AbstractTable* table, uint8_t type = 0);

            //!
            //! Check if an elementary stream carries audio.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries audio.
            //!
            bool isAudio(const DuckContext& duck) const;

            //!
            //! Check if an elementary stream carries video.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries video.
            //!
            bool isVideo(const DuckContext& duck) const;

            //!
            //! Check if an elementary stream carries subtitles.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries subtitles.
            //!
            bool isSubtitles(const DuckContext& duck) const;

            //!
            //! Get the PID class of the stream.
            //! Look at the stream type and the descriptor list.
            //! @param [in] duck TSDuck execution context.
            //! @return The PID class (PIDClass::DATA if unknown component type).
            //!
            PIDClass getClass(const DuckContext& duck) const;

            //!
            //! Try to determine the codec which is used in the stream.
            //! Look at the stream type and the descriptor list.
            //! @param [in] duck TSDuck execution context.
            //! @return The codec type (CodecType::UNDEFINED if unknown).
            //!
            CodecType getCodec(const DuckContext& duck) const;

            //!
            //! Look for a component tag in a stream_identifier_descriptor.
            //! @param [out] tag First component tag found, unmodified if none found.
            //! @return True if a component tag was found.
            //!
            bool getComponentTag(uint8_t& tag) const;

        private:
            // Inaccessible operations.
            Stream() = delete;
            Stream(const Stream&) = delete;
        };

        //!
        //! List of elementary streams, indexed by PID.
        //!
        typedef EntryWithDescriptorsMap<PID, Stream> StreamMap;

        // PMT public members:
        uint16_t       service_id;  //!< Service id aka "program_number".
        PID            pcr_pid;     //!< PID for PCR data.
        DescriptorList descs;       //!< Program-level descriptor list.
        StreamMap      streams;     //!< Map of stream descriptions: key=PID, value=stream_description.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] service_id Service identifier.
        //! @param [in] pcr_pid PID of the PCR. Default: none.
        //!
        PMT(uint8_t  version = 0,
            bool     is_current = true,
            uint16_t service_id = 0,
            PID      pcr_pid = PID_NULL);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PMT(const PMT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        PMT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        PMT& operator=(const PMT& other) = default;

        //!
        //! Search the component PID for a given component tag.
        //! @param [in] tag Component tag to search.
        //! @return The PID of the corresponding component or PID_NULL if not found.
        //!
        PID componentTagToPID(uint8_t tag) const;

        //!
        //! Search the first video PID in the service.
        //! @param [in] duck TSDuck execution context.
        //! @return The first video PID or PID_NULL if none is found.
        //!
        PID firstVideoPID(const DuckContext& duck) const;

        // Inherited methods
        virtual bool isPrivate() const override;
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}

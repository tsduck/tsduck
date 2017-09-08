//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

namespace ts {
    //!
    //! Representation of a Program Map Table (PMT)
    //!
    class TSDUCKDLL PMT : public AbstractLongTable
    {
    public:
        struct Stream;
        //!
        //! List of elementary streams, indexed by PID.
        //!
        typedef std::map<PID, Stream> StreamMap;

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
        PMT(uint8_t version = 0,
            bool is_current = true,
            uint16_t service_id = 0,
            PID pcr_pid = PID_NULL);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //!
        PMT(const BinaryTable& table);

        // Inherited methods
        virtual void serialize(BinaryTable& table) const;
        virtual void deserialize(const BinaryTable& table);
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const;
        virtual void fromXML(XML& xml, const XML::Element* element);

        //!
        //! Description of an elementary stream.
        //!
        struct TSDUCKDLL Stream
        {
            uint8_t        stream_type;  //!< Stream type, one of ST_* (eg ts::ST_MPEG2_VIDEO).
            DescriptorList descs;        //!< Stream-level descriptor list.

            //!
            //! Default constructor.
            //!
            Stream() :
                stream_type(0),
                descs()
            {
            }

            //!
            //! Check if an elementary stream carries audio.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @return True if the elementary stream carries audio.
            //!
            bool isAudio() const;

            //!
            //! Check if an elementary stream carries video.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @return True if the elementary stream carries video.
            //!
            bool isVideo() const;

            //!
            //! Check if an elementary stream carries subtitles.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @return True if the elementary stream carries subtitles.
            //!
            bool isSubtitles() const;
        };

        //!
        //! A static method to display a section.
        //! @param [in,out] display Display engine.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent);
    };
}

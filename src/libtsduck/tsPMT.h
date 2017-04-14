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
//
//  Representation of a Program Map Table (PMT)
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {

    class TSDUCKDLL PMT : public AbstractLongTable
    {
    public:
        // List of elementary streams, indexed by PID
        struct Stream;
        typedef std::map <PID, Stream> StreamMap;

        // PMT public members:
        uint16_t         service_id;  // Service id aka "program_number"
        PID            pcr_pid;     // PID for PCR data
        DescriptorList descs;       // Program-level descriptor list
        StreamMap      streams;     // key=PID, value=stream_description

        // Default constructor:
        PMT (uint8_t version_ = 0,
             bool is_current_ = true,
             uint16_t service_id_ = 0,
             PID pcr_pid_ = PID_NULL);

        // Constructor from a binary table
        PMT (const BinaryTable& table);

        // Inherited methods
        virtual void serialize (BinaryTable& table) const;
        virtual void deserialize (const BinaryTable& table);

        // Description of an elementary stream
        struct TSDUCKDLL Stream
        {
            uint8_t          stream_type;  // One of ST_*
            DescriptorList descs;        // Stream-level descriptor list

            // Constructor
            Stream() : stream_type(0), descs() {}

            // Check if an elementary stream carries audio, video or subtitles.
            // Does not just look at the stream type.
            // Also analyzes the descriptor list for additional information.
            bool isAudio() const;
            bool isVideo() const;
            bool isSubtitles() const;
        };
    };
}

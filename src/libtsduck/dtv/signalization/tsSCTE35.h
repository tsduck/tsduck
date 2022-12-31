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
//!  Common definitions for ANSI / SCTE 35 standard
//!  (splice information for ads insertion).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsVariable.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Registered splice identifier for SCTE 35 (ASCII "CUEI")
    //!
    const uint32_t SPLICE_ID_CUEI = 0x43554549;

    //!
    //! Cue stream type values in cue_identifier_descriptor.
    //!
    enum : uint8_t {
        CUE_INSERT_NULL_SCHEDULE = 0x00, //!< Only splice_insert, splice_null, splice_schedule are allowed in this PID.
        CUE_ALL_COMMANDS         = 0x01, //!< All messages can be used in this PID.
        CUE_SEGMENTATION         = 0x02, //!< This PID carries the time_signal command and the segmentation descriptor.
        CUE_TIERED_SPLICING      = 0x03, //!< Tiered Splicing .
        CUE_TIERED_SEGMENTATION  = 0x04, //!< Tiered Segmentation.
    };

    //!
    //! Splice commands in Splice Information Table.
    //!
    enum : uint8_t {
        SPLICE_NULL                  = 0x00, //!< SpliceNull
        SPLICE_SCHEDULE              = 0x04, //!< SpliceSchedule
        SPLICE_INSERT                = 0x05, //!< SpliceInsert
        SPLICE_TIME_SIGNAL           = 0x06, //!< TimeSignal
        SPLICE_BANDWIDTH_RESERVATION = 0x07, //!< BandwidthReservation
        SPLICE_PRIVATE_COMMAND       = 0xFF, //!< PrivateCommand
    };

    //!
    //! Representation of an SCTE 35 splice_time() structure.
    //! This is a 33-bit PTS value which can be set or unset.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SpliceTime : public Variable<uint64_t>
    {
    private:
        typedef Variable<uint64_t> SuperClass;
    public:
        //!
        //! Deserialize a SpliceTime structure from binary data.
        //! @param [in] data Address of data to deserialize.
        //! @param [in] size Size of data buffer, possibly larger than the SpliceTime structure.
        //! @return Deserialized size, -1 on incorrect data.
        //!
        int deserialize(const uint8_t* data, size_t size);

        //!
        //! Serialize the SpliceTime structure.
        //! @param [in,out] data The SpliceTime structure is serialized at the end of this byte block.
        //!
        void serialize(ByteBlock& data) const;

        //!
        //! Convert the SpliceTime structure to string.
        //! @return The representation string.
        //!
        UString toString() const;

        // Inherited methods.
        //! @cond doxygen
        SpliceTime() = default;
        SpliceTime(const SpliceTime& other) = default;
        SpliceTime& operator=(const SpliceTime& other) { SuperClass::operator=(other); return *this; }
        SpliceTime& operator=(const uint64_t& other) { SuperClass::operator=(other); return *this; }
        virtual ~SpliceTime() override;
        //! @endcond
    };

    //!
    //! Representation of an SCTE 35 private_command() structure.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SplicePrivateCommand
    {
    public:
        // Public fields:
        uint32_t  identifier;     //!< SMPTE identifier.
        ByteBlock private_bytes;  //!< Private command content.

        //!
        //! Constructor
        //! @param [in] id SMPTE identifier.
        //!
        SplicePrivateCommand(uint32_t id = 0);
    };
}

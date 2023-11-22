//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common definitions for ANSI / SCTE 35 standard
//!  (splice information for ads insertion).
//!
//----------------------------------------------------------------------------

#pragma once
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
    class TSDUCKDLL SpliceTime : public std::optional<uint64_t>
    {
    private:
        typedef std::optional<uint64_t> SuperClass;
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
        //! @cond nodoxygen
        SpliceTime() = default;
        SpliceTime(const SpliceTime&) = default;
        SpliceTime& operator=(const SpliceTime&) = default;
        SpliceTime(const SuperClass& other) : SuperClass(other) {}
        SpliceTime& operator=(const SuperClass& other) { SuperClass::operator=(other); return *this; }
        SpliceTime& operator=(const uint64_t& other) { SuperClass::operator=(other); return *this; }
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
        uint32_t  identifier = 0;    //!< SMPTE identifier.
        ByteBlock private_bytes {};  //!< Private command content.

        //!
        //! Constructor
        //! @param [in] id SMPTE identifier.
        //!
        SplicePrivateCommand(uint32_t id = 0) : identifier(id) {}
    };
}

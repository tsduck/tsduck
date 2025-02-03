//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  ISDB-T Information block in a TS packet trailer (204-byte packet).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacketMetadata.h"

namespace ts {

    class DuckContext;

    //!
    //! ISDB-T Information block in a TS packet trailer (204-byte packet).
    //! @ingroup libtsduck mpeg
    //! @see ARIB STD-B31, section 5.5.2
    //!
    class TSDUCKDLL ISDBTInformation
    {
    public:
        // Public information, see ARIB STD-B31, section 5.5.2.
        bool     is_valid = true;                    //!< Reset by constructor from an invalid data area.
        uint8_t  TMCC_identifier = 0;                //!< 2 bits
        bool     buffer_reset_control_flag = false;  //!< 1 bit
        bool     switch_on_control_flag_for_emergency_broadcasting = false;  //!< 1 bit
        bool     initialization_timing_head_packet_flag = false;             //!< 1 bit
        bool     frame_head_packet_flag = false;     //!< 1 bit
        bool     frame_indicator = false;            //!< 1 bit
        uint8_t  layer_indicator = 0;                //!< 4 bits
        uint8_t  count_down_index = 0;               //!< 4 bits
        bool     AC_data_invalid_flag = true;        //!< 1 bit
        uint8_t  AC_data_effective_bytes = 0;        //!< 2 bits
        uint16_t TSP_counter = 0;                    //!< 13 bits
        uint32_t AC_data = 0xFFFFFFFF;               //!< 32 bits, only if AC_data_invalid_flag == false

        static constexpr uint8_t BINARY_SIZE = 8;             //!< Size in bytes of the serialized structure in a TS packet trailer..
        static constexpr uint8_t MAX_ISDBT_LAYER = 0x0F;      //!< Maximum value for an ISDB-T layer indicator.
        static constexpr uint8_t INVALID_ISDBT_LAYER = 0xFF;  //!< Invalid ISDB-T layer indicator value.

        //!
        //! Default constructor.
        //!
        ISDBTInformation() = default;

        //!
        //! Constructor from the auxiliary data in a TS packet metadata.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] mdata Packet metadata.
        //! @param [in] check_standards If true, do nothing if ISDB is not part of @a duck standards.
        //! When false, we assume ISDB.
        //!
        ISDBTInformation(DuckContext& duck, const TSPacketMetadata& mdata, bool check_standards);

        //!
        //! Constructor from the auxiliary data in a TS packet metadata.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] mdata Packet metadata address, can be null.
        //! @param [in] check_standards If true, do nothing if ISDB is not part of @a duck standards.
        //! When false, we assume ISDB.
        //!
        ISDBTInformation(DuckContext& duck, const TSPacketMetadata* mdata, bool check_standards);

        //!
        //! Deserialize a binary area into this object.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] data Address of the binary area to deserialize.
        //! @param [in] size Size in bytes of the binary area to deserialize.
        //! @param [in] check_standards If true, do nothing if ISDB is not part of @a duck standards.
        //! @return True on success, false on error, same value as @a is_valid.
        //!
        bool deserialize(DuckContext& duck, const void* data, size_t size, bool check_standards);

        //!
        //! Display the content of this object.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] strm A standard stream in output mode (text mode).
        //! @param [in] margin Left margin content.
        //!
        void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
    };
}

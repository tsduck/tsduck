//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  BIOP (Broadcast Inter-ORB Protocol) Message structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCC.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! BIOP Message Header.
    //! This is the common header for all BIOP messages in DSM-CC Object Carousel.
    //! @see ISO/IEC 13818-6, Section 8
    //! @see ETSI TR 101 202, Section 4.4
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPMessageHeader
    {
        TS_DEFAULT_COPY_MOVE(BIOPMessageHeader);

    public:
        //!
        //! BIOP magic number ("BIOP" in ASCII).
        //!
        static constexpr uint32_t BIOP_MAGIC = 0x42494F50;

        //!
        //! Standard BIOP version (major).
        //!
        static constexpr uint8_t BIOP_VERSION_MAJOR = 0x01;

        //!
        //! Standard BIOP version (minor).
        //!
        static constexpr uint8_t BIOP_VERSION_MINOR = 0x00;

        //!
        //! Big-endian byte order (always 0x00 per specification).
        //!
        static constexpr uint8_t BIOP_BYTE_ORDER_BIG_ENDIAN = 0x00;

        //!
        //! Standard message type (always 0x00 per specification).
        //!
        static constexpr uint8_t BIOP_MESSAGE_TYPE_STANDARD = 0x00;

        //!
        //! Fixed size of BIOP message header in bytes.
        //!
        static constexpr size_t HEADER_SIZE = 8;

        uint32_t magic = BIOP_MAGIC;                        //!< Magic number, must be 0x42494F50 ("BIOP").
        uint8_t version_major = BIOP_VERSION_MAJOR;         //!< BIOP version major, typically 0x01.
        uint8_t version_minor = BIOP_VERSION_MINOR;         //!< BIOP version minor, typically 0x00.
        uint8_t byte_order = BIOP_BYTE_ORDER_BIG_ENDIAN;    //!< Byte order: 0x00 = big-endian.
        uint8_t message_type = BIOP_MESSAGE_TYPE_STANDARD;  //!< Message type, typically 0x00 for standard message.

        //!
        //! Default constructor.
        //!
        BIOPMessageHeader() = default;

        //!
        //! Check if the header is valid.
        //! @return True if the header has valid magic number and supported version.
        //!
        bool isValid() const;

        //!
        //! Clear the content of the header.
        //!
        void clear();

        //!
        //! Serialize the BIOP message header.
        //! @param [in,out] buf Serialization buffer.
        //! @return True on success, false on error.
        //!
        bool serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the BIOP message header.
        //! @param [in,out] buf Deserialization buffer. The byte order of the buffer
        //! will be set according to the byte_order field in the header.
        //! @return True on success, false on error.
        //!
        bool deserialize(PSIBuffer& buf);

        //!
        //! Display the BIOP message header.
        //! @param [in,out] display Display engine.
        //! @param [in] margin Left margin content.
        //!
        void display(TablesDisplay& display, const UString& margin) const;

        //!
        //! A static method to display a BIOP message header.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the BIOP message header.
        //! @param [in] margin Left margin content.
        //! @return True on success, false on error.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);
    };

    //!
    //! BIOP Object Kind constants.
    //!
    namespace BIOPObjectKind {
        constexpr char DIRECTORY[] = "dir";        //!< Directory object kind (4 bytes with null terminator).
        constexpr char FILE[] = "fil";             //!< File object kind (4 bytes with null terminator).
        constexpr char SERVICE_GATEWAY[] = "srg";  //!< Service Gateway object kind (4 bytes with null terminator).
        constexpr char STREAM[] = "str";           //!< Stream object kind (4 bytes with null terminator).
        constexpr char STREAM_EVENT[] = "ste";     //!< Stream Event object kind (4 bytes with null terminator).
        constexpr size_t SIZE = 4;                 //!< Fixed size of object kind field.
    }  // namespace BIOPObjectKind
}  // namespace ts

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
//!  Definitions for the TLV protocols
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsException.h"

namespace ts {
    //!
    //! Namespace for TLV protocols (Tag / Length / Value)
    //!
    namespace tlv {
        //
        // Basic meta-data in DVB TLV protocols
        //
        typedef uint8_t  VERSION;  //!< Type for TLV protocol version (8 bits).
        typedef uint16_t TAG;      //!< Type for TLV tags (16 bits).
        typedef uint16_t LENGTH;   //!< Type for TLV length fields (16 bits).

        //!
        //! This tag is not used by DVB and can serve as "no value".
        //!
        const TAG NULL_TAG = 0x0000;

        //!
        //! Errors from TLV message analysis.
        //!
        //! An error is associated with a 16-bit "error information".
        //!
        enum Error {
            OK,                       //!< No error.
            UnsupportedVersion,       //!< Offset in message.
            InvalidMessage,           //!< Offset in message.
            UnknownCommandTag,        //!< Offset in message.
            UnknownParameterTag,      //!< Offset in message.
            InvalidParameterLength,   //!< Offset in message.
            InvalidParameterCount,    //!< Parameter tag.
            MissingParameter,         //!< Parameter tag.
        };

        //!
        //! Exception raised by deserialization of messages
        //!
        //! This exception should never been raised by correctly implemented message classes.
        //!
        //! It is raised when:
        //! - A protocol omits to create a message for a command tag it declares.
        //! - A message subclass tries to fetch parameters which are not
        //!   declared in the protocol (or declared with a different size).
        //!
        TS_DECLARE_EXCEPTION(DeserializationInternalError);

        //!
        //! TSDuck specific protocol version.
        //!
        const VERSION TS_PROTOCOL_VERSION = 0x80;

        //!
        //! TSDuck message type values.
        //!
        //! Note that none of the assigned values overlap with the message_type
        //! values which are defined in DVB Simulcrypt protocols. They are
        //! allocated in the "user defined" range. Thus, a generic TLV message
        //! parser can be use for both DVB and TSDuck interfaces.
        //!
        //! Definition of messages:
        //! @code
        //!
        //! MSG_LOG_SECTION
        //! Contains one section.
        //!     Parameter      Count
        //!     PRM_PID        0-1
        //!     PRM_TIMESTAMP  0-1
        //!     PRM_SECTION    1
        //!
        //! MSG_LOG_TABLE
        //! Contains one complete table (no missing section).
        //!     Parameter      Count
        //!     PRM_PID        0-1
        //!     PRM_TIMESTAMP  0-1
        //!     PRM_SECTION    1-n
        //!
        //! @endcode
        //!
        enum {
            MSG_LOG_SECTION = 0xAA01,  //!< Log a section.
            MSG_LOG_TABLE   = 0xAA02,  //!< Log a table.
        };

        //!
        //! TSDuck parameter types values.
        //!
        //! Definition of parameters:
        //! @code
        //!
        //! PRM_PID
        //!     A 2-byte PID value.
        //!
        //! PRM_TIMESTAMP
        //!     A timestamp identifying the occurence of the event. Same format
        //!     as the activation_time in the EIS<=>SCS DVB Simulcrypt protocol:
        //!        year       2 bytes
        //!        month      1 byte
        //!        day        1 byte
        //!        hour       1 byte
        //!        minute     1 byte
        //!        second     1 byte
        //!        hundredth  1 byte
        //!
        //! PRM_SECTION
        //!     A complete section, including header.
        //!
        //! @endcode
        //!
        enum {
            PRM_PID       = 0x0000,  //!< A PID value, 2 bytes.
            PRM_TIMESTAMP = 0x0001,  //!< Timestamp, 8 bytes.
            PRM_SECTION   = 0x0002,  //!< Complete section.
        };
    }
}

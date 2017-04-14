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
//  Definitions for the TLV protocols
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsException.h"

//  TLV Protocols
//  -------------
//  All messages use the same structure as the DVB interfaces defined in the
//  "DVB Simulcrypt Head End" standard, that is to say a TLV protocol.
//  The messages shall have the same generic format as all connection-oriented
//  TLV DVB Simulcrypt protocols and illustrated as follow:
//
//      generic_message
//      {
//          protocol_version      1 byte
//          message_type          2 bytes
//          message_length        2 bytes
//          for (i=0; i < n; i++)
//          {
//              parameter_type    2 bytes
//              parameter_length  2 bytes
//              parameter_value   <parameter_length> bytes
//          }
//      }
//
//  The protocols use the same byte order and parameter order as DVB Simulcrypt
//  protocols: For parameters with a size two or more bytes, the first byte to
//  be transmitted will be the most significant byte. This is commonly known as
//  "big endian" or "MSB first". Parameters do not need to be ordered within the
//  generic message.

namespace ts {
    //!
    //! Namespace for TLV protocols (Tag / Length / Value)
    //!    
    namespace tlv {

        // Basic meta-data in DVB TLV protocols
        // ------------------------------------
        typedef uint8_t  VERSION;
        typedef uint16_t TAG;
        typedef uint16_t LENGTH;

        // This tag is not used by DVB and can serve as "no value".
        const TAG NULL_TAG = 0x0000;


        // Errors from TLV message analysis
        // --------------------------------
        // An error is associated with a 16-bit "error information".

        enum Error {                  // Value of "error information":
            OK,                       // N/A
            UnsupportedVersion,       // offset in message
            InvalidMessage,           // offset in message
            UnknownCommandTag,        // offset in message
            UnknownParameterTag,      // offset in message
            InvalidParameterLength,   // offset in message
            InvalidParameterCount,    // parameter tag
            MissingParameter,         // parameter tag
        };


        // Exception raised by deserialization of messages
        // -----------------------------------------------
        // Should never been raised by correctly implemented message classes.
        // Raised when:
        // - A protocol omits to create a message for a command tag it declares.
        // - A message subclass tries to fetch parameters which are not
        //   declared in the protocol (or declared with a different size).

        tsDeclareException (DeserializationInternalError);


        // TSDuck specific protocol versions
        // -------------------------------
        const uint8_t TS_PROTOCOL_VERSION = 0x80;


        // TSDuck message type values
        // ------------------------
        // Note that none of the assigned values overlap with the message_type
        // values which are defined in DVB Simulcrypt protocols. They are
        // allocated in the "user defined" range. Thus, a generic TLV message
        // parser can be use for both DVB and TSDuck interfaces.

        enum {
            MSG_LOG_SECTION = 0xAA01,
            MSG_LOG_TABLE   = 0xAA02,
        };


        // TSDuck message definitions
        // ------------------------
        //
        // MSG_LOG_SECTION
        // Contains one section.
        //     Parameter      Count
        //     PRM_PID        0-1
        //     PRM_TIMESTAMP  0-1
        //     PRM_SECTION    1
        //
        // MSG_LOG_TABLE
        // Contains one complete table (no missing section).
        //     Parameter      Count
        //     PRM_PID        0-1
        //     PRM_TIMESTAMP  0-1
        //     PRM_SECTION    1-n


        // TSDuck parameter types values
        // ---------------------------
        enum {
            PRM_PID       = 0x0000,  // size: 2 bytes
            PRM_TIMESTAMP = 0x0001,  // size: 8 bytes
            PRM_SECTION   = 0x0002,  // size: variable
        };


        // Parameter definitions
        // ---------------------
        //
        // PRM_PID
        //     A 2-byte PID value.
        //
        // PRM_TIMESTAMP
        //     A timestamp identifying the occurence of the event. Same format
        //     as the activation_time in the EIS<=>SCS DVB Simulcrypt protocol:
        //        year       2 bytes
        //        month      1 byte
        //        day        1 byte
        //        hour       1 byte
        //        minute     1 byte
        //        second     1 byte
        //        hundredth  1 byte
        //
        // PRM_SECTION
        //     A complete sections, including header.
    }
}

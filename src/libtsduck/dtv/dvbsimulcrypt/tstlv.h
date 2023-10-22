//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definitions for the TLV protocols
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsException.h"

namespace ts {
    //!
    //! Namespace for TLV protocols (Tag / Length / Value)
    //!
    namespace tlv {

        class Protocol;
        class Message;

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
        enum Error : uint16_t {
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
    }
}

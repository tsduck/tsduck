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
//!  Abstract base class for TLV messages
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvSerializer.h"
#include "tsUString.h"
#include "tsSafePtr.h"
#include "tsVariable.h"

namespace ts {
    namespace tlv {
        //!
        //! Abstract base class for TLV messages
        //! @ingroup tlv
        //!
        //! All messages use the same structure as the DVB interfaces defined in the
        //! "DVB Simulcrypt Head End" standard, that is to say a TLV protocol.
        //! The messages shall have the same generic format as all connection-oriented
        //! TLV DVB Simulcrypt protocols and illustrated as follow:
        //!
        //! @code
        //!     generic_message
        //!     {
        //!         protocol_version      1 byte
        //!         message_type          2 bytes
        //!         message_length        2 bytes
        //!         for (i=0; i < n; i++)
        //!         {
        //!             parameter_type    2 bytes
        //!             parameter_length  2 bytes
        //!             parameter_value   <parameter_length> bytes
        //!         }
        //!     }
        //! @endcode
        //!
        //! The protocols use the same byte order and parameter order as DVB Simulcrypt
        //! protocols: For parameters with a size two or more bytes, the first byte to
        //! be transmitted will be the most significant byte. This is commonly known as
        //! "big endian" or "MSB first". Parameters do not need to be ordered within the
        //! generic message.
        //!
        class TSDUCKDLL Message
        {
        public:
            //!
            //! Check if the message has a protocol version number.
            //! @return True if the message has a protocol version number.
            //!
            bool hasProtocolVersion() const
            {
                return _has_version;
            }

            //!
            //! Get the protocol version number.
            //! @return The protocol version number.
            //!
            VERSION protocolVersion() const
            {
                return _version;
            }

            //!
            //! Force the protocol version number to another value.
            //! Use with care.
            //! @param [in] version The protocol version number.
            //!
            void forceProtocolVersion(VERSION version)
            {
                _version = version;
            }

            //!
            //! Get the message tag.
            //! @return The message tag.
            //!
            TAG tag() const
            {
                return _tag;
            }

            //!
            //! Serialize the message using a Serializer.
            //! @param [in,out] zer A TLV serializer.
            //!
            void serialize(Serializer& zer) const;

            //!
            //! Virtual destructor.
            //!
            virtual ~Message();

            //!
            //! Dump routine.
            //! Create a string representing the message content.
            //! The implementation in the base class dumps the common fields.
            //! Can be used by subclasses.
            //! @param [in] indent Left indentation size.
            //! @return A string representing the message.
            //!
            virtual UString dump(size_t indent = 0) const;

        protected:
            //!
            //! Protected constructor for subclasses.
            //! @param [in] tag Message tag.
            //!
            Message(TAG tag);

            //!
            //! Protected constructor for subclasses.
            //! @param [in] protocol_version Protocol version.
            //! @param [in] tag Message tag.
            //!
            Message(VERSION protocol_version, TAG tag);

            //!
            //! Parameter serialization.
            //! This protected pure virtual method must be implemented
            //! by subclasses to serialize their parameters.
            //! @param [in,out] zer A TLV serializer.
            //!
            virtual void serializeParameters(Serializer& zer) const = 0;

            //!
            //! Dump a vector of strings (helper routine for subclasses).
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Vector of strings.
            //! @return The formatted string with embedded new-lines.
            //!
            static UString dumpVector(size_t indent, const UString& name, const UStringVector& value);

            //!
            //! Dump an optional byte block (helper routine for subclasses).
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] has_value If false, no value is available, return an empty string.
            //! @param [in] value Byte block.
            //! @param [in] flags Hexa dump flags for ts::Hexa().
            //! @return The formatted string with embedded new-lines.
            //!
            static UString dumpOptional(size_t indent,
                                        const UString& name,
                                        bool has_value,
                                        const ByteBlock& value,
                                        uint32_t flags = UString::HEXA | UString::ASCII);

            //!
            //! Dump an integer value in decimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpDecimal(size_t indent, const UString& name, const INT& value)
            {
                return UString::Format(u"%*s%s = %d\n", {indent, u"", name, value});
            }

            //!
            //! Dump an integer value in hexadecimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpHexa(size_t indent, const UString& name, const INT& value)
            {
                return UString::Format(u"%*s%s = 0x%X\n", {indent, u"", name, value});
            }

            //!
            //! Dump an integer value (helper routine for subclasses).
            //! Signed integer types are dumped in decimal, unsigned types in hexadecimal.
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpInteger(size_t indent, const UString& name, const INT& value)
            {
                return std::numeric_limits<INT>::is_signed ? dumpDecimal(indent, name, value) : dumpHexa(indent, name, value);
            }

            //!
            //! Dump an optional integer value in decimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] has_value If false, no value is available, return an empty string.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalDecimal(size_t indent, const UString& name, bool has_value, const INT& value)
            {
                return has_value ? dumpDecimal(indent, name, value) : u"";
            }

            //!
            //! Dump an optional integer value in decimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalDecimal(size_t indent, const UString& name, const Variable<INT>& value)
            {
                return value.set() ? dumpDecimal(indent, name, value.value()) : u"";
            }

            //!
            //! Dump an optional integer value in hexadecimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] has_value If false, no value is available, return an empty string.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalHexa(size_t indent, const UString& name, bool has_value, const INT& value)
            {
                return has_value ? dumpHexa(indent, name, value) : u"";
            }

            //!
            //! Dump an optional integer value in hexadecimal (helper routine for subclasses).
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalHexa(size_t indent, const UString& name, const Variable<INT>& value)
            {
                return value.set() ? dumpHexa(indent, name, value.value()) : u"";
            }

            //!
            //! Dump an optional integer value (helper routine for subclasses).
            //! Signed integer types are dumped in decimal, unsigned types in hexadecimal.
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] has_value If false, no value is available, return an empty string.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalInteger(size_t indent, const UString& name, bool has_value, const INT& value)
            {
                return has_value ? dumpInteger(indent, name, value) : u"";
            }

            //!
            //! Dump an optional integer value (helper routine for subclasses).
            //! Signed integer types are dumped in decimal, unsigned types in hexadecimal.
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] value Integer value.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpOptionalInteger(size_t indent, const UString& name, const Variable<INT>& value)
            {
                return value.set() ? dumpInteger(indent, name, value.value()) : u"";
            }

            //!
            //! Dump a vector of integer values (helper routine for subclasses).
            //! Signed integer types are dumped in decimal, unsigned types in hexadecimal.
            //! @tparam INT An integer type.
            //! @param [in] indent Left indentation size.
            //! @param [in] name Parameter name.
            //! @param [in] val Vector of integer values.
            //! @param [in] toString Optional function to convert an @a INT value into a string.
            //! @return The formatted string with embedded new-lines.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static UString dumpVector(size_t indent, const UString& name, const std::vector<INT>& val, UString(*toString)(INT) = nullptr)
            {
                UString s;
                for (auto i : val) {
                    if (toString == nullptr) {
                        s += dumpInteger(indent, name, i);
                    }
                    else {
                        s += UString::Format(u"%*s%s = %s\n", {indent, u"", name, toString(i)});
                    }
                }
                return s;
            }

        private:
            // Unreachable constructor.
            Message() = delete;

            // Private members
            bool    _has_version;
            VERSION _version;
            TAG     _tag;
        };

        //!
        //! Safe pointer for TLV messages (not thread-safe).
        //!
        typedef SafePtr<Message, NullMutex> MessagePtr;

        //!
        //! Safe pointer for TLV messages (thread-safe).
        //!
        typedef SafePtr<Message, Mutex> MessagePtrMT;
    }
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            TS_RULE_OF_FIVE(Message, );
        protected:
            //!
            //! Alias for the superclass of subclasses.
            //!
            using superclass = Message;

        public:
            //!
            //! Check if the message has a protocol version number.
            //! @return True if the message has a protocol version number.
            //!
            bool hasProtocolVersion() const { return _has_version; }

            //!
            //! Get the protocol version number.
            //! @return The protocol version number.
            //!
            VERSION protocolVersion() const { return _version; }

            //!
            //! Force the protocol version number to another value.
            //! Use with care.
            //! @param [in] version The protocol version number.
            //!
            void forceProtocolVersion(VERSION version) { _version = version; }

            //!
            //! Get the message tag.
            //! @return The message tag.
            //!
            TAG tag() const { return _tag; }

            //!
            //! Serialize the message using a Serializer.
            //! @param [in,out] zer A TLV serializer.
            //!
            void serialize(Serializer& zer) const;

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
            static UString dumpOptionalDecimal(size_t indent, const UString& name, const std::optional<INT>& value)
            {
                return value.has_value() ? dumpDecimal(indent, name, value.value()) : u"";
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
            static UString dumpOptionalHexa(size_t indent, const UString& name, const std::optional<INT>& value)
            {
                return value.has_value() ? dumpHexa(indent, name, value.value()) : u"";
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
            static UString dumpOptionalInteger(size_t indent, const UString& name, const std::optional<INT>& value)
            {
                return value.has_value() ? dumpInteger(indent, name, value.value()) : u"";
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
            static UString dumpVector(size_t indent, const UString& name, const std::vector<INT>& val, UString(*toString)(INT) = nullptr);

        private:
            // Unreachable constructor.
            Message() = delete;

            // Private members
            bool    _has_version = false;
            VERSION _version = 0;
            TAG     _tag = 0;
        };

        //!
        //! Safe pointer for TLV messages (not thread-safe).
        //!
        typedef SafePtr<Message, ts::null_mutex> MessagePtr;

        //!
        //! Safe pointer for TLV messages (thread-safe).
        //!
        typedef SafePtr<Message, std::mutex> MessagePtrMT;
    }
}

//!
//! A macro to add in the declaration of a ts::tlv::Message subclass without version.
//! @param classname Name of the enclosing class.
//!
#define TS_UNVERSIONED_TLV_MESSAGE(classname)                        \
    public:                                                          \
        /** Default constructor. */                                  \
        classname();                                                 \
        /** Constructor from a message factory. */                   \
        /** @param [in] fact Message factory. */                     \
        classname(const ts::tlv::MessageFactory& fact);              \
        /* Implementation of Message. */                             \
        virtual ts::UString dump(size_t indent = 0) const override;  \
    protected:                                                       \
        /* Implementation of Message. */                             \
        virtual void serializeParameters(ts::tlv::Serializer&) const override

//!
//! A macro to add in the declaration of a ts::tlv::Message subclass with version.
//! @param classname Name of the enclosing class.
//! @param tag Message tag.
//!
#define TS_VERSIONED_TLV_MESSAGE(classname, tag)                     \
    private:                                                         \
        classname() = delete;                                        \
    public:                                                          \
        /** Constructor with version. */                             \
        /** @param [in] version Protocol version. */                 \
        classname(ts::tlv::VERSION version) :                        \
            superclass(version, tag) {}                              \
        /** Constructor with version from protocol. */               \
        /** @param [in] proto Protocol definition. */                \
        classname(const ts::tlv::Protocol& proto) :                  \
            classname(proto.version()) {}                            \
        /** Constructor from a message factory. */                   \
        /** @param [in] fact Message factory. */                     \
        classname(const ts::tlv::MessageFactory& fact);              \
        /* Implementation of Message. */                             \
        virtual ts::UString dump(size_t indent = 0) const override;  \
    protected:                                                       \
        /* Implementation of Message. */                             \
        virtual void serializeParameters(ts::tlv::Serializer&) const override


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Dump a vector of integer values (helper routine for subclasses).
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString ts::tlv::Message::dumpVector(size_t indent, const UString& name, const std::vector<INT>& val, UString(*toString)(INT))
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

#endif // DOXYGEN

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
//!  Element of an argument list with mixed integer and string types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsUChar.h"

namespace ts {

    class UString;

    //!
    //! Define an element of an argument list with mixed integer and string types.
    //!
    //! This class is typically used as element in an std::initializer_list.
    //! This mechanism is used by ts::UString::Format() for instance.
    //!
    //! An instance of ArgMix may reference external data. The lifetime of the
    //! pointed data must be longer than the ArgMix instance. This is the case
    //! for a std::initializer_list<ArgMix> which is used as parameter to Format().
    //! But this is not guaranteed in other usages.
    //!
    class TSDUCKDLL ArgMix
    {
    public:
        //!
        //! Type of an argument.
        //! Only integers and strings are supported. Integers are stored as 32 or 64 bits only.
        //!
        //! Binary encoding:
        //! - xx0 = signed/char,    xx1 = unsigned/uchar
        //! - x0x = 32-bit/pointer, x1x = 64-bit/class
        //! - 0xx = integer,        1xx = string
        //!
        enum Type {
            INT32    = 0x00,  // Signed 32-bit integer.
            UINT32   = 0x01,  // Unsigned 32-bit integer.
            INT64    = 0x02,  // Signed 64-bit integer.
            UINT64   = 0x03,  // Unsigned 64-bit integer.
            CHARPTR  = 0x04,  // Pointer to a nul-terminated string of 8-bit characters.
            UCHARPTR = 0x05,  // Pointer to a nul-terminated string of 16-bit characters.
            STRING   = 0x06,  // Reference to a std::string.
            USTRING  = 0x07   // Reference to a ts::UString.
        };

        //!
        //! Default constructor.
        //!
        ArgMix() : _type(INT32), _size(0), _value(int32_t(0)) {}
        //!
        //! Constructor from a nul-terminated string of 8-bit characters.
        //! @param [in] s Address of nul-terminated string.
        //!
        ArgMix(const char* s) : _type(CHARPTR), _size(0), _value(s) {}
        //!
        //! Constructor from a nul-terminated string of 16-bit characters.
        //! @param [in] s Address of nul-terminated string.
        //!
        ArgMix(const UChar* s) : _type(UCHARPTR), _size(0), _value(s) {}
        //!
        //! Constructor from a C++ string of 8-bit characters.
        //! @param [in] s Reference to a C++ string.
        //!
        ArgMix(const std::string& s) : _type(STRING), _size(0), _value(s) {}
        //!
        //! Constructor from a C++ string of 16-bit characters.
        //! @param [in] s Reference to a C++ string.
        //!
        ArgMix(const UString& s) : _type(USTRING), _size(0), _value(s) {}
        //!
        //! Constructor from an integer or enum type.
        //! @param [in] i Integer value of the ArgMix. Internally stored as a 32-bit or 64-bit integer.
        //!
        template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type* = nullptr>
        ArgMix(T i) : _type(storage_type<T>::value), _size(sizeof(i)), _value((typename storage_type<T>::type)(i)) {}

        //!
        //! Get the argument data type.
        //! @return The argument data type.
        //!
        Type type() const { return _type; }
        //!
        //! Check if the argument value is an integer.
        //! @return True if the argument value is an integer.
        //!
        bool isInt() const { return (_type & 0x04) == 0x00; }
        //!
        //! Check if the argument value is a signed integer.
        //! @return True if the argument value is a signed integer.
        //!
        bool isSigned() const { return (_type & 0x05) == 0x00; }
        //!
        //! Check if the argument value is an unsigned integer.
        //! @return True if the argument value is an unsigned integer.
        //!
        bool isUnsigned() const { return (_type & 0x05) == 0x01; }
        //!
        //! Check if the argument value is a string of any type.
        //! @return True if the argument value is a string (CHARPTR, UCHARPTR, STRING, USTRING).
        //!
        bool isString() const { return (_type & 0x04) == 0x04; }
        //!
        //! Check if the argument value is a string of 8-bit characters.
        //! @return True if the argument value is a string of 8-bit characters (CHARPTR or STRING).
        //!
        bool isString8() const { return (_type & 0x05) == 0x04; }
        //!
        //! Check if the argument value is a string of 16-bit characters.
        //! @return True if the argument value is a string of 16-bit characters (UCHARPTR or USTRING).
        //!
        bool isString16() const { return (_type & 0x05) == 0x05; }
        //!
        //! Get the original integer size in bytes of the argument data.
        //! @return The original integer size in bytes of the argument data or zero for a string.
        //!
        size_t size() const { return _size; }

        //!
        //! Get the argument data value as an integer.
        //! @tparam INT An integer type.
        //! @return The argument data as an integer value of type @a INT or zero for a string.
        //!
        template <typename INT>
        INT toInteger() const;
        //!
        //! Get the argument data value as a 32-bit signed integer.
        //! @return The argument data as an integer value or zero for a string.
        //!
        int32_t toInt32() const { return toInteger<int32_t>(); }
        //!
        //! Get the argument data value as a 32-bit unsigned integer.
        //! @return The argument data as an integer value or zero for a string.
        //!
        uint32_t toUInt32() const { return toInteger<uint32_t>(); }
        //!
        //! Get the argument data value as a 64-bit signed integer.
        //! @return The argument data as an integer value or zero for a string.
        //!
        int64_t toInt64() const { return toInteger<int64_t>(); }
        //!
        //! Get the argument data value as a 64-bit unsigned integer.
        //! @return The argument data as an integer value or zero for a string.
        //!
        uint64_t toUInt64() const { return toInteger<uint64_t>(); }
        //!
        //! Get the argument data value as a nul-terminated string of 8-bit characters.
        //! @return Address of the nul-terminated string for CHARPTR or STRING, an empty string for other data types.
        //!
        const char* toCharPtr() const;
        //!
        //! Get the argument data value as a nul-terminated string of 16-bit characters.
        //! @return Address of the nul-terminated string for UCHARPTR or USTRING, an empty string for other data types.
        //!
        const UChar* toUCharPtr() const;
        //!
        //! Get the argument data value as constant reference to a C++ string of 8-bit characters.
        //! @return Reference to the string for STRING, to an empty string for other data types.
        //!
        const std::string& toString() const { return _type == STRING && _value.string != 0 ? *_value.string : empty; }
        //!
        //! Get the argument data value as constant reference to a C++ string of 16-bit characters.
        //! @return Reference to the string for USTRING, to an empty string for other data types.
        //!
        const UString& toUString() const { return _type == USTRING && _value.ustring != 0 ? *_value.ustring : uempty; }

    private:
        // There must be exactly one value in Type per overlay in Value.
        union Value {
            int32_t            int32;
            uint32_t           uint32;
            int64_t            int64;
            uint64_t           uint64;
            const char*        charptr;
            const UChar*       ucharptr;
            const std::string* string;
            const UString*     ustring;

            Value(int32_t i)            : int32(i) {}
            Value(uint32_t i)           : uint32(i) {}
            Value(int64_t i)            : int64(i) {}
            Value(uint64_t i)           : uint64(i) {}
            Value(const char* s)        : charptr(s) {}
            Value(const UChar* s)       : ucharptr(s) {}
            Value(const std::string& s) : string(&s) {}
            Value(const UString& s)     : ustring(&s) {}
        };

        Type   _type;  // which overlay to use in _value
        size_t _size;  // for integer type, informative only
        Value  _value; // actual value of the 

        // Used to return references to constant empty strings.
        static const std::string empty;
        static const ts::UString uempty;

        // Warning: The rest of this class is carefully crafted template meta-programming (aka. Black Magic).
        // It is correct, it works, but it is not immediately easy to understand. So, do not modify it if you
        // are not 100% sure to have understood it and you know what you are doing. You have been warned...

        // The meta-type "underlying_type_impl" is the basis for underlying_type.
        // The default definition, when ISENUM is false is T and applies to integer types.
        template<bool ISENUM, typename T>
        struct underlying_type_impl {
            typedef T type;
        };

        // The following specialized definition applies to enum types.
        template<typename T>
        struct underlying_type_impl<true,T> {
            typedef typename std::underlying_type<T>::type type;
        };

        // The meta-type "underlying_type" is a generalization of std::underlying_type
        // which works on integer types as well. The underlying type of an integer
        // type is the type itself.
        template<typename T>
        struct underlying_type {
            typedef typename underlying_type_impl<std::is_enum<T>::value, T>::type type;
        };

        // The meta-type "storage_type" defines the characteristics of the type which is
        // used to store an integer or enum types in an ArgMix.
        template <typename T>
        struct storage_type {

            // The meta-type "type" is the storage type, namely one of int32_t, uint32_t, int64_t, uint64_t.
            typedef typename std::conditional<
                std::is_signed<typename underlying_type<T>::type>::value,
                typename std::conditional<(sizeof(T) > 4), int64_t, int32_t>::type,
                typename std::conditional<(sizeof(T) > 4), uint64_t, uint32_t>::type
            >::type type;

            // The meta-type "type_constant" defines the ArgMix::Type value for the type T.
            typedef typename std::conditional<
                std::is_signed<typename underlying_type<T>::type>::value,
                typename std::conditional<(sizeof(T) > 4), std::integral_constant<Type,INT64>, std::integral_constant<Type,INT32>>::type,
                typename std::conditional<(sizeof(T) > 4), std::integral_constant<Type,UINT64>, std::integral_constant<Type,UINT32>>::type
            >::type type_constant;

            // The "value" is the ArgMix::Type value for the type T.
            static constexpr Type value = type_constant::value;
        };

    };
}

#include "tsArgMixTemplate.h"

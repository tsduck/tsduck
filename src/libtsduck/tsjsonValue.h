//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Simple and basic implementation of a JSON value.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! Namespace for JSON (JavaScript Object Notation) classes.
    //!
    namespace json {

        class Value;

        //!
        //! Smart pointer to a JSON value (not thread-safe).
        //!
        typedef SafePtr<Value, NullMutex> ValuePtr;

        //!
        //! Definition of the type of a value.
        //! JSON defines 7 types of value.
        //!
        enum Type {
            TypeNull,     //!< null literal.
            TypeTrue,     //!< true literal.
            TypeFalse,    //!< false literal.
            TypeString,   //!< string value.
            TypeNumber,   //!< number value (integer only for now).
            TypeObject,   //!< structured object.
            TypeArray     //!< array of values.
        };

        //!
        //! Abstract base class of a JSON value.
        //!
        //! A JSON document is composed of one value (usually of type "object" or "array").
        //! JSON is defined by RFC 8259.
        //!
        //! This implementation is simple and basic. Currently, it can only deserialize
        //! a JSON value from text. The JSON value is read-only and serialization is
        //! not yet implemented.
        //!
        //! Fetching a non-existent element, field or whatever always fails silently
        //! by returning a reference to a "null" value. Thus, it is possible to access
        //! a complex chain of references without caring about intermediate errors.
        //! At the end, a "null" value will be returned.
        //!
        //! Floating-point values are not yet implemented. Reading a number always
        //! give a null object.
        //!
        //! @see http://json.org/
        //! @see https://tools.ietf.org/html/rfc8259
        //!
        class TSDUCKDLL Value
        {
        public:
            //!
            //! Virtual destructor.
            //!
            virtual ~Value() {}
            //!
            //! Get the JSON value type.
            //! @return The JSON value type.
            //!
            virtual Type type() const = 0;
            //!
            //! Check if this instance a is JSON null literal.
            //! @return True if this instance a is JSON null literal.
            //!
            virtual bool isNull() const { return false; }
            //!
            //! Check if this instance a is JSON true literal.
            //! @return True if this instance a is JSON true literal.
            //!
            virtual bool isTrue() const { return false; }
            //!
            //! Check if this instance a is JSON false literal.
            //! @return True if this instance a is JSON false literal.
            //!
            virtual bool isFalse() const { return false; }
            //!
            //! Check if this instance a is JSON number.
            //! @return True if this instance a is JSON number.
            //!
            virtual bool isNumber() const { return false; }
            //!
            //! Check if this instance a is JSON string.
            //! @return True if this instance a is JSON string.
            //!
            virtual bool isString() const { return false; }
            //!
            //! Check if this instance a is JSON object.
            //! @return True if this instance a is JSON object.
            //!
            virtual bool isObject() const { return false; }
            //!
            //! Check if this instance a is JSON array.
            //! @return True if this instance a is JSON array.
            //!
            virtual bool isArray() const { return false; }
            //!
            //! Convert this instance to a boolean.
            //! @return The boolean value of this instance or @a defaultValue if no conversion is possible.
            //! For numbers, zero is false, all other values are true. For string, "yes", "true", "on" and
            //! non-zero integers are true; "no", "false", "off" and zero integers are false.
            //!
            virtual bool toBoolean(bool defaultValue = false) const { return defaultValue; }
            //!
            //! Convert this instance to an integer.
            //! @return The integer value of this instance or @a defaultValue if no conversion is possible.
            //! Strings containing integers are converted.
            //!
            virtual int64_t toInteger(int64_t defaultValue = 0) const { return defaultValue; }
            //!
            //! Convert this instance to a string.
            //! @return The string value of this instance or @a defaultValue if no conversion is possible.
            //!
            virtual UString toString(const UString& defaultValue = UString()) const { return defaultValue; }
            //!
            //! Get the "size" of the value.
            //! @return The size of the value: the number of characters for strings, of fields for objects,
            //! of elements for arrays, zero for other types.
            //!
            virtual size_t size() const { return 0; }
            //!
            //! Get an element of an array.
            //! @param [in] index Index to fetch in the array.
            //! @return For a JSON array, return a constant reference to the given element.
            //! When out of bound or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual const Value& at(size_t index) const;
            //!
            //! Get the value of an object field.
            //! @param [in] name Field name.
            //! @return For a JSON object, return a constant reference to the given element.
            //! When the field does not exist or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual const Value& value(const UString& name) const;
        };

        //!
        //! Implementation of a JSON null literal.
        //!
        class TSDUCKDLL Null : public Value
        {
        public:
            // Implementation of ts::json::Value.
            virtual Type type() const override { return TypeNull; }
            virtual bool isNull() const override { return true; }
        };

        //!
        //! Implementation of a JSON true literal.
        //!
        class TSDUCKDLL True : public Value
        {
        public:
            // Implementation of ts::json::Value.
            virtual Type type() const override { return TypeTrue; }
            virtual bool isTrue() const override { return true; }
            virtual bool toBoolean(bool defaultValue = false) const override { return true; }
            virtual int64_t toInteger(int64_t defaultValue = 0) const override { return 1; }
            virtual UString toString(const UString& defaultValue = UString()) const override { return u"true"; }
        };

        //!
        //! Implementation of a JSON false literal.
        //!
        class TSDUCKDLL False : public Value
        {
        public:
            // Implementation of ts::json::Value.
            virtual Type type() const override { return TypeFalse; }
            virtual bool isFalse() const override { return true; }
            virtual bool toBoolean(bool defaultValue = false) const override { return false; }
            virtual int64_t toInteger(int64_t defaultValue = 0) const override{ return 0; }
            virtual UString toString(const UString& defaultValue = UString()) const override { return u"false"; }
        };

        //!
        //! Implementation of a JSON number.
        //! Currently, floating-point numbers are not implemented.
        //! All JSON numbers are integers or null.
        //!
        class TSDUCKDLL Number : public Value
        {
        public:
            //!
            //! Constructor.
            //! @param [in] value Initial integer value.
            //!
            Number(int64_t value = 0) : _value(value) {}

            // Implementation of ts::json::Value.
            virtual Type type() const override { return TypeNumber; }
            virtual bool isNumber() const override { return true; }
            virtual bool toBoolean(bool defaultValue = false) const override { return false; }
            virtual int64_t toInteger(int64_t defaultValue = 0) const override { return _value; }
            virtual UString toString(const UString& defaultValue = UString()) const override { return UString::Decimal(_value, 0, true, UString()); }

        private:
            int64_t _value;
        };

        //!
        //! Implementation of a JSON string.
        //!
        class TSDUCKDLL String : public Value
        {
        public:
            //!
            //! Constructor.
            //! @param [in] value Initial string value.
            //!
            String(const UString& value = UString()) : _value(value) {}

            // Implementation of ts::json::Value.
            virtual Type type() const override { return TypeString; }
            virtual bool isString() const override { return true; }
            virtual bool toBoolean(bool defaultValue = false) const override;
            virtual int64_t toInteger(int64_t defaultValue = 0) const override;
            virtual UString toString(const UString& defaultValue = UString()) const override { return _value; }
            virtual size_t size() const override { return _value.size(); }

        private:
            UString _value;
        };

        //!
        //! A general-purpose constant null JSON value.
        //!
        extern const Null NullValue;
    }
}

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
//!  Abstract base class of a JSON value.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjson.h"

namespace ts {

    class TextFormatter;

    namespace json {
        //!
        //! Abstract base class of a JSON value.
        //!
        //! A JSON document is composed of one value (usually of type "object" or "array").
        //! JSON is defined by RFC 8259.
        //!
        //! This implementation is simple and basic.
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
        //! @ingroup json
        //!
        class TSDUCKDLL Value
        {
        public:
            //!
            //! Default constructor.
            //!
            Value() = default;
            //!
            //! Virtual destructor.
            //!
            virtual ~Value() = default;
            //!
            //! Get the JSON value type.
            //! @return The JSON value type.
            //!
            virtual Type type() const = 0;
            //!
            //! Format the value as JSON text.
            //! @param [in,out] output The output object to format.
            //!
            virtual void print(TextFormatter& output) const = 0;
            //!
            //! Format the value as JSON text.
            //! @param [in] indent Indentation width of each level.
            //! @param [in,out] report Where to report errors.
            //! @return The formatted JSON text.
            //!
            virtual UString printed(size_t indent = 2, Report& report = NULLREP) const;
            //!
            //! Save the value as a JSON file.
            //! @param [in] fileName Name of the JSON file to save.
            //! @param [in] indent Indentation width of each level.
            //! @param [in] stdOutputIfEmpty If true and if @a fileName is empty or "-", writes to the standard output.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            virtual bool save(const UString& fileName, size_t indent = 2, bool stdOutputIfEmpty = false, Report& report = NULLREP);
            //!
            //! Check if this instance a is JSON null literal.
            //! @return True if this instance a is JSON null literal.
            //!
            virtual bool isNull() const;
            //!
            //! Check if this instance a is JSON true literal.
            //! @return True if this instance a is JSON true literal.
            //!
            virtual bool isTrue() const;
            //!
            //! Check if this instance a is JSON false literal.
            //! @return True if this instance a is JSON false literal.
            //!
            virtual bool isFalse() const;
            //!
            //! Check if this instance a is JSON number.
            //! @return True if this instance a is JSON number.
            //!
            virtual bool isNumber() const;
            //!
            //! Check if this instance a is JSON string.
            //! @return True if this instance a is JSON string.
            //!
            virtual bool isString() const;
            //!
            //! Check if this instance a is JSON object.
            //! @return True if this instance a is JSON object.
            //!
            virtual bool isObject() const;
            //!
            //! Check if this instance a is JSON array.
            //! @return True if this instance a is JSON array.
            //!
            virtual bool isArray() const;
            //!
            //! Convert this instance to a boolean.
            //! @param [in] defaultValue Default value to return when no conversion is possible.
            //! @return The boolean value of this instance or @a defaultValue if no conversion is possible.
            //! For numbers, zero is false, all other values are true. For string, "yes", "true", "on" and
            //! non-zero integers are true; "no", "false", "off" and zero integers are false.
            //!
            virtual bool toBoolean(bool defaultValue = false) const;
            //!
            //! Convert this instance to an integer.
            //! @param [in] defaultValue Default value to return when no conversion is possible.
            //! @return The integer value of this instance or @a defaultValue if no conversion is possible.
            //! Strings containing integers are converted.
            //!
            virtual int64_t toInteger(int64_t defaultValue = 0) const;
            //!
            //! Convert this instance to a string.
            //! @param [in] defaultValue Default value to return when no conversion is possible.
            //! @return The string value of this instance or @a defaultValue if no conversion is possible.
            //!
            virtual UString toString(const UString& defaultValue = UString()) const;
            //!
            //! Clear the content of the value;
            //!
            virtual void clear();
            //!
            //! Get the "size" of the value.
            //! @return The size of the value: the number of characters for strings, of fields for objects,
            //! of elements for arrays, zero for other types.
            //!
            virtual size_t size() const;
            //!
            //! Get all field names in an object.
            //! @param [out] names Receive the list of field names.
            //!
            virtual void getNames(UStringList& names) const;
            //!
            //! Get the value of an object field (const version).
            //! @param [in] name Field name.
            //! @return For a JSON object, return a constant reference to the given element.
            //! When the field does not exist or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual const Value& value(const UString& name) const;
            //!
            //! Get the value of an object field (modifiable version).
            //! @param [in] name Field name.
            //! @param [in] create If true and the field does not exist, create it.
            //! @param [in] type When the field is created, provide the object type.
            //! @return For a JSON object, return a reference to the given element.
            //! When the field does not exist or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual Value& value(const UString& name, bool create = false, Type type = Type::Object);
            //!
            //! Get the value of an object field (pointer version).
            //! Must be a non-const object since we can potentially modify the field.
            //! @param [in] name Field name.
            //! @return For a JSON object, return a safe pointer to the given element.
            //! When the field does not exist or for other types of JSON values, return a null pointer.
            //!
            virtual ValuePtr valuePtr(const UString& name);
            //!
            //! Remove a field from an object.
            //! @param [in] name Field name.
            //!
            virtual void remove(const UString& name);
            //!
            //! Extract a field from an object.
            //! The field is removed but its previous content is returned.
            //! @param [in] name Field name.
            //! @return A smart pointer to the extracted content or a null pointer if the field does not exist.
            //!
            virtual ValuePtr extract(const UString& name);
            //!
            //! Add a field into an object.
            //! @param [in] name Field name.
            //! @param [in] value Smart pointer to a JSON value. The pointed object is shared.
            //!
            virtual void add(const UString& name, const ValuePtr& value);
            //!
            //! Add a number field into an object.
            //! @param [in] name Field name.
            //! @param [in] value Field value.
            //!
            virtual void add(const UString& name, int64_t value);
            //!
            //! Add a string field into an object.
            //! @param [in] name Field name.
            //! @param [in] value Field value.
            //!
            virtual void add(const UString& name, const UString& value);
            //!
            //! Get an element of an array (const version).
            //! @param [in] index Index to fetch in the array.
            //! @return For a JSON array, return a constant reference to the given element.
            //! When out of bound or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual const Value& at(size_t index) const;
            //!
            //! Get an element of an array (modifiable version).
            //! @param [in] index Index to fetch in the array.
            //! @return For a JSON array, return a reference to the given element.
            //! When out of bound or for other types of JSON values, return a reference to a null JSON.
            //!
            virtual Value& at(size_t index);
            //!
            //! Set an element of an array.
            //! @param [in] value Smart pointer to a JSON value. The pointed object is shared.
            //! @param [in] index Index to fetch in the array. If out of bound, the @a value is added at the end of the array.
            //! @return The actual index of the added value.
            //!
            virtual size_t set(const ValuePtr& value, size_t index = std::numeric_limits<size_t>::max());
            //!
            //! Set a number element of an array.
            //! @param [in] value Value to set.
            //! @param [in] index Index in the array. If out of bound, the @a value is added at the end of the array.
            //! @return The actual index of the added value.
            //!
            virtual size_t set(int64_t value, size_t index = std::numeric_limits<size_t>::max());
            //!
            //! Set a string element of an array.
            //! @param [in] value Value to set.
            //! @param [in] index Index in the array. If out of bound, the @a value is added at the end of the array.
            //! @return The actual index of the added value.
            //!
            virtual size_t set(const UString& value, size_t index = std::numeric_limits<size_t>::max());
            //!
            //! Erase elements from an array.
            //! @param [in] index Index to erase in the array. Ignored if out of bound.
            //! @param [in] count Number of elements to erase.
            //!
            virtual void erase(size_t index, size_t count = 1);
            //!
            //! Extract an element from an array.
            //! The element is removed but its previous content is returned.
            //! @param [in] index Index to erase in the array. Ignored if out of bound.
            //! @return A smart pointer to the extracted content or a null pointer if the element does not exist.
            //!
            virtual ValuePtr extractAt(size_t index);
            //!
            //! Query a JSON value using a composite path.
            //! @param [in] path Full path from this object to final object.
            //! May contain object field names and array indexes. Example: "obj1.arr[3].obj2".
            //! If this object is an array, the path must start with a index (example: "[3].obj2").
            //! If the path is empty, this object is returned.
            //! @return A reference to the final object or a reference to a null JSON if not found.
            //!
            virtual const Value& query(const UString& path) const;
            //!
            //! Query a JSON value using a composite path, creating intermediate objects when necessary.
            //! @param [in] path Full path from this object to final object.
            //! May contain object field names and array indexes. Example: "obj1.arr[3].obj2".
            //! If this object is an array, the path must start with a index (example: "[3].obj2").
            //! When an array index is empty and @a create is true, a new element is created at the
            //! end of the array (example: "obj1.arr[].obj2").
            //! If the path is empty, this object is returned.
            //! @param [in] create If true create all missing intermediate objects.
            //! @param [in] type When the final field is created, provide the object type.
            //! @return A reference to the final object. When the field does not exist or could
            //! not be created, return a reference to a null JSON.
            //!
            virtual Value& query(const UString& path, bool create = false, Type type = Type::Object);
        };
    }
}

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
//!  Simple and basic implementation of a JSON value.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsSafePtr.h"
#include "tsTextParser.h"
#include "tsNullReport.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Namespace for JSON (JavaScript Object Notation) classes.
    //!
    namespace json {

        class Value;

        //!
        //! Safe pointer to a JSON value (not thread-safe).
        //!
        typedef SafePtr<Value, NullMutex> ValuePtr;

        //!
        //! A vector of safe pointers to JSON values.
        //!
        typedef std::vector<ValuePtr> ValuePtrVector;

        //!
        //! A list of safe pointers to JSON values.
        //!
        typedef std::list<ValuePtr> ValuePtrList;

        //!
        //! Definition of the type of a value.
        //! JSON defines 7 types of value.
        //!
        enum class Type {
            Null,     //!< Null literal.
            True,     //!< True literal.
            False,    //!< False literal.
            String,   //!< String value.
            Number,   //!< Number value (integer only for now).
            Object,   //!< Structured object.
            Array     //!< Array of values.
        };

        //!
        //! Enumeration description of ts::json::Type.
        //!
        TSDUCKDLL extern const Enumeration TypeEnum;

        //!
        //! Create a JSON value by type.
        //! @param [in] type The type JSON object to create.
        //! @param [in] value Optional value. For TypeString, use this value.
        //! For TypeNumber, convert the string into a number (zero on error).
        //! For all other types, the value is ignored.
        //! @return A smart pointer to the created JSON value.
        //!
        TSDUCKDLL ValuePtr Factory(Type type, const UString& value = UString());

        //!
        //! Create a boolean JSON value.
        //! @param [in] value A boolean value.
        //! @return A smart pointer to the created JSON value, either a True or False literal.
        //!
        TSDUCKDLL ValuePtr Bool(bool value);

        //!
        //! Parse a JSON value (typically an object or array).
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in] lines List of text lines forming the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, const UStringList& lines, Report& report = NULLREP);

        //!
        //! Parse a JSON value (typically an object or array).
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in] text The text forming the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, const UString& text, Report& report = NULLREP);

        //!
        //! Parse a JSON value (typically an object or array).
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in,out] parser A text parser.
        //! @param [in] jsonOnly If true, the parsed text shall not contain anything else than
        //! the JSON value (except white spaces). If false, on output, the position of the parser
        //! is right after the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, TextParser& parser, bool jsonOnly, Report& report = NULLREP);

        //!
        //! Load a JSON value (typically an object or array) from a text file.
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in] filename The name of the JSON file. If empty or "-", the standard input is used.
        //! If @a filename starts with "{" or "[", this is considered as "inline JSON content".
        //! The document is loaded from this string instead of reading a file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool LoadFile(ValuePtr& value, const UString& filename, Report& report = NULLREP);

        //!
        //! Load a JSON value (typically an object or array) from an open text stream.
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in,out] strm A standard text stream in input mode.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool LoadStream(ValuePtr& value, std::istream& strm, Report& report = NULLREP);

        //!
        //! Check if a "file name" is in fact inline JSON content instead of a file name.
        //! We currently only test if the name starts with '{' or '['. This will fail if
        //! we port TSDuck to OpenVMS, however...
        //! @param [in] name A file name string.
        //! @return True if @a name contains inline JSON content, false otherwise.
        //!
        TSDUCKDLL bool IsInlineJSON(const UString& name);
    }
}

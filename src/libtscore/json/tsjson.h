//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Simple and basic implementation of a JSON value.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNullReport.h"
#include "tsNames.h"

namespace ts {

    class TextParser;
    class TextFormatter;

    //!
    //! Namespace for JSON (JavaScript Object Notation) classes.
    //!
    namespace json {
        class Value;
        class Object;
        class Array;
        class String;
        class Number;
        class True;
        class False;
        class Null;
        class RunningDocument;
    }
}

namespace ts::json {
    //!
    //! Safe pointer to a JSON value (not thread-safe).
    //! @ingroup json
    //!
    using ValuePtr = std::shared_ptr<Value>;

    //!
    //! A vector of safe pointers to JSON values.
    //! @ingroup json
    //!
    using ValuePtrVector = std::vector<ValuePtr>;

    //!
    //! A list of safe pointers to JSON values.
    //! @ingroup json
    //!
    using ValuePtrList = std::list<ValuePtr>;

    //!
    //! Definition of the type of a value.
    //! JSON defines 7 types of value.
    //! @ingroup json
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
    //! @ingroup json
    //! @return A constant reference to the enumeration description.
    //!
    TSCOREDLL const Names& TypeEnum();

    //!
    //! Create a JSON value by type.
    //! @ingroup json
    //! @param [in] type The type JSON object to create.
    //! @param [in] value Optional value. For TypeString, use this value.
    //! For TypeNumber, convert the string into a number (zero on error).
    //! For all other types, the value is ignored.
    //! @return A smart pointer to the created JSON value.
    //!
    TSCOREDLL ValuePtr Factory(Type type, const UString& value = UString());

    //!
    //! Create a boolean JSON value.
    //! @ingroup json
    //! @param [in] value A boolean value.
    //! @return A smart pointer to the created JSON value, either a True or False literal.
    //!
    TSCOREDLL ValuePtr Bool(bool value);

    //!
    //! Parse a JSON value (typically an object or array).
    //! @ingroup json
    //! @param [out] value A smart pointer to the parsed JSON value (null on error).
    //! @param [in] lines List of text lines forming the JSON value.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool Parse(ValuePtr& value, const UStringList& lines, Report& report = NULLREP);

    //!
    //! Parse a JSON value (typically an object or array).
    //! @ingroup json
    //! @param [out] value A smart pointer to the parsed JSON value (null on error).
    //! @param [in] text The text forming the JSON value.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool Parse(ValuePtr& value, const UString& text, Report& report = NULLREP);

    //!
    //! Parse a JSON value (typically an object or array).
    //! @ingroup json
    //! @param [out] value A smart pointer to the parsed JSON value (null on error).
    //! @param [in,out] parser A text parser.
    //! @param [in] jsonOnly If true, the parsed text shall not contain anything else than
    //! the JSON value (except white spaces). If false, on output, the position of the parser
    //! is right after the JSON value.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool Parse(ValuePtr& value, TextParser& parser, bool jsonOnly, Report& report = NULLREP);

    //!
    //! Load a JSON value (typically an object or array) from a text file.
    //! @ingroup json
    //! @param [out] value A smart pointer to the parsed JSON value (null on error).
    //! @param [in] filename The name of the JSON file. If empty or "-", the standard input is used.
    //! If @a filename starts with "{" or "[", this is considered as "inline JSON content".
    //! The document is loaded from this string instead of reading a file.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool LoadFile(ValuePtr& value, const UString& filename, Report& report = NULLREP);

    //!
    //! Load a JSON value (typically an object or array) from an open text stream.
    //! @ingroup json
    //! @param [out] value A smart pointer to the parsed JSON value (null on error).
    //! @param [in,out] strm A standard text stream in input mode.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool LoadStream(ValuePtr& value, std::istream& strm, Report& report = NULLREP);

    //!
    //! Check if a "file name" is in fact inline JSON content instead of a file name.
    //! We currently only test if the name starts with '{' or '['. This will fail if
    //! we port TSDuck to OpenVMS, however...
    //! @ingroup json
    //! @param [in] name A file name string.
    //! @return True if @a name contains inline JSON content, false otherwise.
    //!
    TSCOREDLL bool IsInlineJSON(const UString& name);
}

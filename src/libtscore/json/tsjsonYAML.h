//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Conversion between JSON and YAML (experimental).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjson.h"

namespace ts::json {
    //!
    //! Conversion between JSON and YAML (experimental).
    //! @ingroup libtscore json
    //!
    class TSCOREDLL YAML
    {
    public:
        //!
        //! Format a JSON value as YAML text.
        //! @param [in,out] out The text formatter which is used to output.
        //! @param [in] value The JSON value to format.
        //! @param [in] with_headers When true, add the standard YAML header ("---") and trailer ("...").
        //!
        static void PrintAsYAML(TextFormatter& out, const json::Value& value, bool with_headers = false);

        //!
        //! String type, according to YAML usage.
        //! There is a strict ordering: each level is a superset of the preceding one.
        //!
        enum class StringType {
            KEY,         //!< Can be used as mapping key.
            SCALAR,      //!< Can be used without quotes.
            MULTI_LINE,  //!< Can be used in a multi-line string (after '|') without quotes.
            QUOTED       //!< Requires quotes.
        };

        //!
        //! Determine the type of a string.
        //! @param [in] str The string to check.
        //! @return The type of the string.
        //! @see https://www.yaml.info/learn/quote.html
        //!
        //! Warning: if the returned type is MULTI_LINE, be sure to check if the string
        //! ends with a new line (use "|") or without (use "|-").
        //!
        static StringType GetStringType(const UString& str);

        //!
        //! Build a quoted string for YAML.
        //! @param [in] str The string to quote.
        //! @return The quoted string.
        //!
        static UString QuotedString(const UString& str);

        //!
        //! Format a string as a literal block scalar.
        //! Do not print the last end of line.
        //! @param [in,out] out The text formatter which is used to output.
        //! @param [in] str The string to format.
        //!
        static void PrintLiteralBlock(TextFormatter& out, const UString& str);

    private:
        // Recursive implementation of PrintAsYAML().
        static void PrintValueAsYAML(TextFormatter& out, const json::Value& value);
    };
}

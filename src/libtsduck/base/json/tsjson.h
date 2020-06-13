//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsTextFormatter.h"
#include "tsTextParser.h"
#include "tsNullReport.h"

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
        //! Parse a JSON value (typically an object or array.
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in] lines List of text lines forming the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, const UStringList& lines, Report& report = NULLREP);

        //!
        //! Parse a JSON value (typically an object or array.
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in] text The text forming the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, const UString& text, Report& report = NULLREP);

        //!
        //! Parse a JSON value (typically an object or array.
        //! @param [out] value A smart pointer to the parsed JSON value (null on error).
        //! @param [in,out] parser A text parser.
        //! @param [in] jsonOnly If true, the parsed text shall not contain anything else than
        //! the JSON value (except white spaces). If false, on output, the position of the parser
        //! is right after the JSON value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool Parse(ValuePtr& value, TextParser& parser, bool jsonOnly, Report& report = NULLREP);
    }
}

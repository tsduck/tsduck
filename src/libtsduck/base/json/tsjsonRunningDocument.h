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
//!  Representation of a "running" JSON document which is displayed on the fly.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjson.h"
#include "tsTextFormatter.h"

namespace ts {
    namespace json {
        //!
        //! Representation of a "running" JSON document which is displayed on the fly.
        //! @ingroup json
        //!
        //! The idea is to display or save a JSON document containing an array of values
        //! which is built value by value without waiting for the end of the document.
        //!
        //! The JSON document is initially displayed with whatever can be displayed
        //! and the final array is left open so that new values can be added later.
        //!
        //! The "open" array can be the root value of the JSON document or inside one
        //! or more levels of objects.
        //!
        class TSDUCKDLL RunningDocument
        {
            TS_NOCOPY(RunningDocument);
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit RunningDocument(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            ~RunningDocument();

            //!
            //! Initialize the running document.
            //! @param [in] root Root JSON value.
            //! - If @a root is a null pointer, assume an empty array.
            //! - If @a root is an array, its current elements are printed and the array is left open.
            //! - If @a root is an object, it is recursively searched until the first array is found.
            //!   Everything else is printed and this array is left open. If no array is found, this is an error.
            //! - If @a root is any other type of JSON value, this is an error.
            //! @param [in] fileName Output file name to create. When empty or "-", @a strm is used for output.
            //! @param [in,out] strm The default output text stream when @a fileName is empty or "-".
            //! The referenced stream object must remain valid as long as this object.
            //! @return True on success, false on error.
            //!
            bool open(const ValuePtr& root, const UString& fileName = UString(), std::ostream& strm = std::cout);

            //!
            //! Add one JSON value in the open array of the running document.
            //! @param [in] value The JSON value to add.
            //!
            void add(const Value& value);

            //!
            //! Close the running document.
            //! If the JSON structure is still open, it is closed.
            //! The output file, if any, is closed.
            //!
            void close();

        private:
            TextFormatter _text;         // The text formatter.
            bool          _open_array;   // The array is open.
            bool          _empty_array;  // The open array is currently empty.
            size_t        _obj_count;    // Number of parent objects.

            // Look for a JSON array in a tree. Return true if one is found, false otherwise.
            // Build a path of objects, one per level. The last one is the array.
            bool searchArray(const ValuePtr& root, ValuePtrVector& path);
        };
    }
}

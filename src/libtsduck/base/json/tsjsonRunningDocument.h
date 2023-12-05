//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            explicit RunningDocument(Report& report = NULLREP) : _text(report) {}

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
            bool open(const ValuePtr& root, const fs::path& fileName = fs::path(), std::ostream& strm = std::cout);

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
            TextFormatter _text {};             // The text formatter.
            bool          _open_array = false;  // The array is open.
            bool          _empty_array = false; // The open array is currently empty.
            size_t        _obj_count = 0;       // Number of parent objects.

            // Look for a JSON array in a tree. Return true if one is found, false otherwise.
            // Build a path of objects, one per level. The last one is the array.
            bool searchArray(const ValuePtr& root, ValuePtrVector& path);
        };
    }
}

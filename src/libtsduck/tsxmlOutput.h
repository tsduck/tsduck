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
//!  Format and print and XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"

namespace ts {
    namespace xml {
        //!
        //! Format and print and XML document.
        //!
        //! This class is used to print a complete XML document or a subset of it.
        //! Output encoding is automatically converted to UTF-8.
        //! Continuation is supported, meaning that a file can remain open
        //! and XML node are added as they come.
        //!
        class TSDUCKDLL Output
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit Output(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            virtual ~Output();

            //!
            //! Get the margin size for outer-most elements.
            //! @return The margin size for outer-most elements.
            //!
            size_t marginSize() const { return _margin; }

            //!
            //! Set the margin size for outer-most elements.
            //! @param [in] margin The margin size for outer-most elements.
            //! @return A reference to this object.
            //!
            Output& setMarginSize(size_t margin) { _margin = margin; return *this; }

            //!
            //! Get the indent size for inner elements.
            //! @return The indent size for inner elements.
            //!
            size_t indentSize() const { return _indent; }

            //!
            //! Set the indent size for inner elements.
            //! @param [in] indent The indent size for inner elements.
            //! @return A reference to this object.
            //!
            Output& setIndentSize(size_t indent) { _indent = indent; return *this; }

            //!
            //! Get the compact mode, for one-line output.
            //! @return The compact mode, for one-line output.
            //!
            bool compact() const { return _compact; }

            //!
            //! Set the compact mode, for one-line output.
            //! @param [in] compact The compact mode, for one-line output.
            //! @return A reference to this object.
            //!
            Output& setCompact(bool compact) { _compact = compact; return *this; }

            //!
            //! Set output to an open text stream.
            //! @param [in,out] strm The output text stream.
            //! The referenced stream object must remain valid as long as this object.
            //! @return A reference to this object.
            //!
            Output& setStream(std::ostream& strm);

            //!
            //! Set output to a text file.
            //! @param [in] fileName Output file name.
            //! @return True on success, false on error.
            //!
            bool setFile(const UString& fileName);

            //!
            //! Set output to an internal string buffer. 
            //! @return A reference to this object.
            //! @see getString()
            //!
            Output& setString();

            //!
            //! Retrieve the current content of the internal string buffer. 
            //! Must be called after setString() and before close().
            //! @param [out] str Returned string containing the formatted XML document.
            //! @return True on success, false if there is no internal string buffer.
            //! @see setString()
            //!
            bool getString(UString& str);

            //!
            //! Return the current content of the internal string buffer. 
            //! Must be called after setString() and before close().
            //! @return The string containing the formatted XML document.
            //! @see getString()
            //!
            UString toString();

            //!
            //! Check if the Output is open to some output.
            //! @return True if the Output is open.
            //!
            bool isOpen() const;

            //!
            //! Close the current output.
            //! Depending on the output mode:
            //! - The external stream is no longer referenced.
            //! - The external file is closed.
            //! - The internal string buffer is emptied.
            //!
            void close();

            //!
            //! Push one indentation level, typically when formatting child items.
            //!
            void pushIndent() { _curMargin += _indent; }

            //!
            //! Pop one indentation level, typically when formatting back to parent.
            //!
            void popIndent() { _curMargin -= std::min(_indent, _curMargin); }

            //!
            //! Get a reference to the output text stream.
            //! @return A reference to the output text stream.
            //!
            std::ostream& stream() const { return *_out; }

            //!
            //! Output a new line on a stream if not in compact mode.
            //! @return A reference to the output text stream.
            //!
            std::ostream& newLine() const;

            //!
            //! Output an indentation margin on a stream if not in compact mode.
            //! @return A reference to the output text stream.
            //!
            std::ostream& margin() const { return spaces(_curMargin); }

            //!
            //! Output spaces on a stream if not in compact mode.
            //! @param [in] count Number of spaces to print.
            //! @return A reference to the output text stream.
            //!
            std::ostream& spaces(size_t count) const;

        private:
            Report&            _report;     // Where to report errors.
            std::ofstream      _outFile;    // Own stream when output to a file we created.
            std::ostringstream _outString;  // Internal string buffer.
            std::ostream*      _out;        // Address of current output stream.
            size_t             _margin;     // Margin size for outer-most element.
            size_t             _indent;     // Indent size for inner elements.
            bool               _compact;    // If true, compact one-line output.
            size_t             _curMargin;  // Current margin size.

            // Unaccessible operations.
            Output(const Output&) = delete;
            Output& operator=(const Output&) = delete;
        };
    }
}

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
//!  XML parser class (private to TSDuck library).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    namespace xml {

        class Node;

        //!
        //! A class which parses an XML document.
        //!
        class Parser
        {
        public:
            //!
            //! Constructor.
            //! @param [in] lines Reference to a list of text lines forming the XML document.
            //! The lifetime of the referenced list must equals or exceeds the lifetime of the parser.
            //! @param [in,out] report Where to report errors.
            //!
            Parser(const UStringList& lines, Report& report);

            //!
            //! Check end of file.
            //! @return True if the parser reached the end of file.
            //!
            bool eof() const { return _curLine == _endLine; }

            //!
            //! Get the current line number.
            //! @return The current line number.
            //!
            size_t lineNumber() const { return _curLineNumber; }

            //!
            //! Skip all whitespaces, including end of lines.
            //! Note that the optional BOM at start of an UTF-8 file has already been removed by the UTF-16 conversion.
            //! @return Always true.
            //!
            bool skipWhiteSpace();

            //!
            //! Check if the current position in the document matches a string.
            //! @param [in] str A string to check at the current position in the document.
            //! @param [in] skipIfMatch If true and @a str matches the current position, skip it in the document.
            //! @param [in] cs Case sensitivity of the comparision.
            //! @return True if @a str matches the current position in the document.
            //!
            bool match(const UChar* str, bool skipIfMatch, CaseSensitivity cs = CASE_SENSITIVE);

            //!
            //! Identify the next token in the document.
            //! @return A new node or zero at end.
            //! The returned node is not yet linked to its parent and siblings.
            //! When not zero, the parser is located after the tag which identified the node ("<?", "<!--", etc.)
            //! Return zero either at end of document or before a "</" sequence.
            //!
            Node* identify();

            //!
            //! Parse text up to a given token.
            //! @param [out] result Returned parsed text.
            //! @param [in] endToken Stop when this token is found. Do not include @a endToken
            //! in returned string. 
            //! @param [in] skipIfMatch If true, skip @a endToken in the parser.
            //! @param [in] translateEntities If true, translate HTML entities in the text.
            //! @return True on success, false if @a endToken was not found.
            //!
            bool parseText(UString& result, const UString endToken, bool skipIfMatch, bool translateEntities);

            //!
            //! Check if the parser is at the start of a name.
            //! @return True if the parser is at the start of a name.
            //!
            bool isAtNameStart() const;

            //!
            //! Parse a tag name.
            //! @param [out] name Returned parsed name.
            //! @return True on success, false if no name was not found.
            //!
            bool parseName(UString& name);

        private:
            Report&                           _report;
            UStringList::const_iterator       _curLine;
            const UStringList::const_iterator _endLine;
            size_t                            _curLineNumber;
            size_t                            _curIndex;  // index in current line.

            // Check if a character is suitable for starting a name or in the middle of a name.
            static bool IsNameStartChar(UChar c)
            {
                return IsAlpha(c) || c == u':' || c == u'_';
            }
            static bool IsNameChar(UChar c)
            {
                return IsNameStartChar(c) || IsDigit(c) || c == u'.' || c == u'-';
            }

            // Unaccessible operations.
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator=(const Parser&) = delete;
        };
    }
}

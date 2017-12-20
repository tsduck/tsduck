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
//!  Simple text parser class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! A support class for applications which parse various text formats.
    //!
    class TextParser
    {
    public:
        //!
        //! Constructor.
        //! The document to parse is empty.
        //! @param [in,out] report Where to report errors.
        //!
        TextParser(Report& report);

        //!
        //! Constructor.
        //! @param [in] lines Reference to a list of text lines forming the document.
        //! The lifetime of the referenced list must equals or exceeds the lifetime of the parser.
        //! @param [in,out] report Where to report errors.
        //!
        TextParser(const UStringList& lines, Report& report);

        //!
        //! Constructor.
        //! @param [in] text Document text to parse with embedded new-line characters.
        //! @param [in,out] report Where to report errors.
        //!
        TextParser(const UString& text, Report& report);

        //!
        //! Destructor.
        //!
        virtual ~TextParser() {}

        //!
        //! Clear the document in the parser.
        //!
        virtual void clear();

        //!
        //! Load the document to parse from a list of lines.
        //! @param [in] lines Reference to a list of text lines forming the document.
        //! The lifetime of the referenced list must equals or exceeds the lifetime of the parser.
        //!
        void loadDocument(const UStringList& lines);

        //!
        //! Load the document to parse.
        //! @param [in] text Document text to parse with embedded new-line characters.
        //!
        void loadDocument(const UString& text);

        //!
        //! Load the document to parse from a text file.
        //! @param [in] fileName Name of the file to load.
        //! @return True on success, false on failure.
        //!
        bool loadFile(const UString& fileName);

        //!
        //! Check end of file.
        //! @return True if the parser reached the end of file.
        //!
        bool eof() const;

        //!
        //! Check end of line.
        //! @return True if the parser reached the end of line.
        //!
        bool eol() const;

        //!
        //! Rewind to start of document.
        //!
        void rewind();

        //!
        //! A class which describes a position in the document.
        //!
        class Position
        {
        private:
            // Constructors.
            Position() = delete;
            Position(const UStringList&);

            // Everything is private to the application.
            // Only TextParser can use it.
            friend class TextParser;

            const UStringList*          _lines;
            UStringList::const_iterator _curLine;
            size_t                      _curLineNumber;
            size_t                      _curIndex;
        };

        //!
        //! Save the position in the document.
        //! @return The current position in the document.
        //!
        Position position() const { return _pos; }

        //!
        //! Restore a previous position in the document.
        //! @param [in] pos Previous position to restore.
        //! @return True on success, false on error.
        //!
        bool seek(const Position& pos);

        //!
        //! Get the current line number.
        //! @return The current line number.
        //!
        size_t lineNumber() const { return _pos._curLineNumber; }

        //!
        //! Skip all whitespaces, including end of lines.
        //! Note that the optional BOM at start of an UTF-8 file has already been removed by the UTF-16 conversion.
        //! @return Always true.
        //!
        bool skipWhiteSpace();

        //!
        //! Skip to next line.
        //! @return Always true.
        //!
        bool skipLine();

        //!
        //! Check if the current position in the document matches a string.
        //! @param [in] str A string to check at the current position in the document.
        //! @param [in] skipIfMatch If true and @a str matches the current position, skip it in the document.
        //! @param [in] cs Case sensitivity of the comparision.
        //! @return True if @a str matches the current position in the document.
        //!
        bool match(const UString& str, bool skipIfMatch, CaseSensitivity cs = CASE_SENSITIVE);

        //!
        //! Parse text up to a given token.
        //! @param [out] result Returned parsed text.
        //! @param [in] endToken Stop when this token is found. Do not include @a endToken in returned string. 
        //! @param [in] skipIfMatch If true, skip @a endToken in the parser.
        //! @param [in] translateEntities If true, translate HTML entities in the text.
        //! @return True on success, false if @a endToken was not found.
        //!
        virtual bool parseText(UString& result, const UString endToken, bool skipIfMatch, bool translateEntities);

        //!
        //! Check if a character is suitable for starting a @e name.
        //! The concept of @e name depends on the type of text which is parsed.
        //! The default implementation is compatible with XML: a name starts with a letter,
        //! underscore or colon. For other text syntaxes, derive the TextParser class and
        //! override this method.
        //! @param [in] c The character to check.
        //! @return True if @a c is suitable for starting a name.
        //!
        virtual bool isNameStartChar(UChar c) const;

        //!
        //! Check if a character is suitable in the middle of a name.
        //! The concept of @e name depends on the type of text which is parsed.
        //! The default implementation is compatible with XML: a name contains letters,
        //! digits, underscores, colons, dots and dashes. For other text syntaxes,
        //! derive the TextParser class and override this method.
        //! @param [in] c The character to check.
        //! @return True if @a c is suitable in the middle of a name.
        //!
        virtual bool isNameChar(UChar c) const;

        //!
        //! Check if the parser is at the start of a name.
        //! @return True if the parser is at the start of a name.
        //!
        virtual bool isAtNameStart() const;

        //!
        //! Parse a name.
        //! @param [out] name Returned parsed name.
        //! @return True on success, false if no name was not found.
        //!
        virtual bool parseName(UString& name);

    private:
        TS_UNUSED Report& _report;
        UStringList       _lines;
        Position          _pos;

        // Unaccessible operations.
        TextParser() = delete;
        TextParser(const TextParser&) = delete;
        TextParser& operator=(const TextParser&) = delete;
    };
}

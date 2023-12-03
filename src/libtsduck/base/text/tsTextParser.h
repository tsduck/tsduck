//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Simple text parser class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"

namespace ts {
    //!
    //! A support class for applications which parse various text formats.
    //! @ingroup cpp
    //!
    class TextParser
    {
        TS_NOBUILD_NOCOPY(TextParser);
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
        virtual ~TextParser();

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
        bool loadFile(const fs::path& fileName);

        //!
        //! Load the document to parse from a text stream.
        //! @param [in,out] strm A standard text stream in input mode.
        //! @return True on success, false on error.
        //!
        bool loadStream(std::istream& strm);

        //!
        //! Save the document to parse to a text file.
        //! @param [in] fileName Name of the file to save.
        //! @return True on success, false on failure.
        //!
        bool saveFile(const fs::path& fileName);

        //!
        //! Save the document to parse to a text stream.
        //! @param [in,out] strm A standard text stream in output mode.
        //! @return True on success, false on error.
        //!
        bool saveStream(std::ostream& strm);

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
        //! Check if a character is suitable for starting an XML @e name.
        //! An XML name starts with a letter, underscore or colon.
        //! @param [in] c The character to check.
        //! @return True if @a c is suitable for starting an XML name.
        //!
        virtual bool isXMLNameStartChar(UChar c) const;

        //!
        //! Check if a character is suitable in the middle of an XML name.
        //! An XML name contains letters, digits, underscores, colons, dots and dashes.
        //! @param [in] c The character to check.
        //! @return True if @a c is suitable in the middle of an XML name.
        //!
        virtual bool isXMLNameChar(UChar c) const;

        //!
        //! Check if the parser is at the start of an XML name.
        //! @return True if the parser is at the start of a name.
        //!
        virtual bool isAtXMLNameStart() const;

        //!
        //! Parse an XML name.
        //! @param [out] name Returned parsed name.
        //! @return True on success, false if no name was not found.
        //!
        virtual bool parseXMLName(UString& name);

        //!
        //! Check if the parser is at the start of a number (digit or minus sign).
        //! @return True if the parser is at the start of a number.
        //!
        virtual bool isAtNumberStart() const;

        //!
        //! Parse a numeric literal.
        //! @param [out] str Returned numeric literal
        //! @param [in] allowHexa If true, hexadecimal integers "0x..." are allowed.
        //! @param [in] allowFloat It true, floating point values "[-]digits[.digits][e|E[+|-]digits]" are allowed.
        //! @return True on success, false if no string literal was not found.
        //!
        virtual bool parseNumericLiteral(UString& str, bool allowHexa = false, bool allowFloat = true);

        //!
        //! Parse a string literal.
        //! A string literal is enclosed in simple or double quotes.
        //! Any similar quotation mark is considered part of the string when it is preceded
        //! by a backslash character.
        //! @param [out] str Returned string literal, including the first and last quote.
        //! When the method returns true (success), it is consequently guaranteed that
        //! the length of the returned string is greater than or equal to 2.
        //! @param [in] requiredQuote If defined as single or double quote, accept only
        //! that type of quote. Otherwise, accept any of the two.
        //! @return True on success, false if no string literal was not found.
        //!
        virtual bool parseStringLiteral(UString& str, UChar requiredQuote = CHAR_NULL);

        //!
        //! Parse a JSON string literal.
        //! @param [out] str Returned decoded value of the string literal.
        //! @return True on success, false if no string literal was not found.
        //!
        virtual bool parseJSONStringLiteral(UString& str);

    private:
        Report&     _report;
        UStringList _lines;
        Position    _pos;
    };
}

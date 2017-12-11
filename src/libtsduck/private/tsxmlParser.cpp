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

#include "tsxmlParser.h"
#include "tsxmlAttribute.h"
#include "tsxmlComment.h"
#include "tsxmlDeclaration.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsxmlUnknown.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Parser, a class which parses an XML document.
//----------------------------------------------------------------------------

ts::xml::Parser::Parser(const UStringList& lines, Report& report) :
    Report(report.maxSeverity()),
    _report(report),
    _curLine(lines.begin()),
    _endLine(lines.end()),
    _curLineNumber(1)
{
}


//----------------------------------------------------------------------------
// Parser report interface.
//----------------------------------------------------------------------------

void ts::xml::Parser::writeLog(int severity, const UString& msg)
{
    _report.log(severity, u"line %d: %s", {_curLineNumber, msg});
}

void ts::xml::Parser::errorAtLine(size_t lineNumber, const UChar* fmt, const std::initializer_list<ArgMixIn> args)
{
    const size_t previousLine = _curLineNumber;
    _curLineNumber = lineNumber;
    log(Severity::Error, fmt, args);
    _curLineNumber = previousLine;
}


//----------------------------------------------------------------------------
// Parser: Skip all whitespaces.
//----------------------------------------------------------------------------

void ts::xml::Parser::skipWhiteSpace()
{
    while (_curLine != _endLine) {
        // Skip spaces in current line.
        while (_curIndex < _curLine->length() && IsSpace(_curLine->at(_curIndex))) {
            _curIndex++;
        }
        // Stop if not at end of line (non-space character found).
        if (_curIndex < _curLine->length()) {
            return;
        }
        // Move to next line.
        _curLine++;
        _curLineNumber++;
        _curIndex = 0;
    }
}


//----------------------------------------------------------------------------
// Parser: Check if the current position in the document matches a string.
//----------------------------------------------------------------------------

bool ts::xml::Parser::match(const UChar* str, bool skipIfMatch, CaseSensitivity cs)
{
    if (_curLine == _endLine) {
        // Already at end of document.
        return false;
    }

    size_t index = _curIndex;
    while (*str != 0) {
        if (index >= _curLine->length() || !Match(*str++, _curLine->at(index++), cs)) {
            // str does not match
            return false;
        }
    }

    if (*str != 0) {
        // End of line encountered before match
        return false;
    }
    if (skipIfMatch) {
        _curIndex = index;
    }
    return true;
}


//----------------------------------------------------------------------------
// Parser: Identify the next token in the document.
//----------------------------------------------------------------------------

ts::xml::Node* ts::xml::Parser::identify()
{
    // Save the current state in case we realize that the leading spaces are part of the token.
    const UStringList::const_iterator savedCurLine = _curLine;
    const size_t savedCurLineNumber = _curLineNumber;
    const size_t savedCurIndex = _curIndex;

    // Skip all white spaces until next token.
    skipWhiteSpace();

    // Error at end of document.
    if (eof()) {
        return 0;
    }

    // Check each expected token.
    if (match(u"<?", true, CASE_SENSITIVE)) {
        return new Declaration(_curLineNumber);
    }
    else if (match(u"<!--", true, CASE_SENSITIVE)) {
        return new Comment(_curLineNumber);
    }
    else if (match(u"<![CDATA[", true, CASE_INSENSITIVE)) {
        return new Text(_curLineNumber, true);
    }
    else if (match(u"<!", true, CASE_SENSITIVE)) {
        // Should be a DTD, we ignore it.
        return new Unknown(_curLineNumber);
    }
    else if (match(u"<", true, CASE_SENSITIVE)) {
        return new Element(_curLineNumber);
    }
    else {
        // This must be a text node. Revert skipped spaces, they are part of the text.
        _curLine = savedCurLine;
        _curLineNumber = savedCurLineNumber;
        _curIndex = savedCurIndex;
        return new Text(_curLineNumber, false);
    }
}


//----------------------------------------------------------------------------
// Parse a tag name.
//----------------------------------------------------------------------------

bool ts::xml::Parser::parseName(UString& name)
{
    name.clear();

    // We may have spaces between "<" and the name. May we? We accept it anyway.
    skipWhiteSpace();

    // Check that the next character is valid to start a name.
    if (eof() || !IsNameStartChar(_curLine->at(_curIndex))) {
        return false;
    }

    // Get the name.
    UChar c;
    while (_curIndex < _curLine->length() && IsNameChar(c = _curLine->at(_curIndex))) {
        name.append(c);
        _curIndex++;
    }

    // Handle end of line.
    if (_curIndex >= _curLine->length()) {
        _curLine++;
        _curLineNumber++;
        _curIndex = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Parse text up to a given token.
//----------------------------------------------------------------------------

bool ts::xml::Parser::parseText(UString& result, const UString endToken, bool translateEntities)
{
    result.clear();
    bool found = false;

    // Loop on all lines until the end token is found.
    while (!found && _curLine != _endLine) {

        // Search for the end token in current line.
        const size_t end = _curLine->find(endToken, _curIndex);
        if (end == UString::NPOS) {
            // End token not found, include the complete end of line.
            result.append(*_curLine, _curIndex);
            result.append(LINE_FEED);
            _curLine++;
            _curLineNumber++;
            _curIndex = 0;
        }
        else {
            // Found end token, stop here.
            result.append(*_curLine, _curIndex, end - _curIndex);
            _curIndex = end + endToken.length();
            found = true;
        }
    }

    // Translate HTML entities in the result if required.
    if (translateEntities) {
        result.convertFromHTML();
    }

    return found;
}

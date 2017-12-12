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
    _report(report),
    _curLine(lines.begin()),
    _endLine(lines.end()),
    _curLineNumber(1),
    _curIndex(0)
{
}


//----------------------------------------------------------------------------
// Parser: Skip all whitespaces.
//----------------------------------------------------------------------------

bool ts::xml::Parser::skipWhiteSpace()
{
    while (_curLine != _endLine) {
        // Skip spaces in current line.
        while (_curIndex < _curLine->length() && IsSpace(_curLine->at(_curIndex))) {
            _curIndex++;
        }
        // Stop if not at end of line (non-space character found).
        if (_curIndex < _curLine->length()) {
            return true;
        }
        // Move to next line.
        _curLine++;
        _curLineNumber++;
        _curIndex = 0;
    }
    return true;
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

    // Stop at end of document or before "</".
    if (eof() || match(u"</", false)) {
        return 0;
    }

    // Check each expected token.
    if (match(u"<?", true)) {
        return new Declaration(_report, _curLineNumber);
    }
    else if (match(u"<!--", true)) {
        return new Comment(_report, _curLineNumber);
    }
    else if (match(u"<![CDATA[", true, CASE_INSENSITIVE)) {
        return new Text(_report, _curLineNumber, true);
    }
    else if (match(u"<!", true)) {
        // Should be a DTD, we ignore it.
        return new Unknown(_report, _curLineNumber);
    }
    else if (match(u"<", true)) {
        return new Element(_report, _curLineNumber);
    }
    else {
        // This must be a text node. Revert skipped spaces, they are part of the text.
        _curLine = savedCurLine;
        _curLineNumber = savedCurLineNumber;
        _curIndex = savedCurIndex;
        return new Text(_report, _curLineNumber, false);
    }
}


//----------------------------------------------------------------------------
// Check if the parser is at the start of a name.
//----------------------------------------------------------------------------

bool ts::xml::Parser::isAtNameStart() const
{
    return _curLine != _endLine && _curIndex < _curLine->length() && IsNameStartChar(_curLine->at(_curIndex));
}


//----------------------------------------------------------------------------
// Parse a tag name.
//----------------------------------------------------------------------------

bool ts::xml::Parser::parseName(UString& name)
{
    name.clear();

    // Check that the next character is valid to start a name.
    if (!isAtNameStart()) {
        return false;
    }

    // Get the name.
    UChar c;
    while (_curIndex < _curLine->length() && IsNameChar(c = _curLine->at(_curIndex))) {
        name.append(c);
        _curIndex++;
    }
    return true;
}


//----------------------------------------------------------------------------
// Parse text up to a given token.
//----------------------------------------------------------------------------

bool ts::xml::Parser::parseText(UString& result, const UString endToken, bool skipIfMatch, bool translateEntities)
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
            _curIndex = skipIfMatch ? end + endToken.length() : end;
            found = true;
        }
    }

    // Translate HTML entities in the result if required.
    if (translateEntities) {
        result.convertFromHTML();
    }

    return found;
}

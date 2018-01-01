//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsTextParser.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors & destructors.
//----------------------------------------------------------------------------

ts::TextParser::TextParser(Report& report) :
    _report(report),
    _lines(),
    _pos(_lines)
{
}

ts::TextParser::TextParser(const UStringList& lines, Report& report) :
    TextParser(report)
{
    loadDocument(lines);
}

ts::TextParser::TextParser(const UString& text, Report& report) :
    TextParser(report)
{
    loadDocument(text);
}

ts::TextParser::Position::Position(const UStringList& textLines) :
    _lines(&textLines),
    _curLine(textLines.begin()),
    _curLineNumber(1),
    _curIndex(0)
{
}


//----------------------------------------------------------------------------
// Clear the document in the parser.
//----------------------------------------------------------------------------

void ts::TextParser::clear()
{
    _lines.clear();
    _pos = Position(_lines);
}


//----------------------------------------------------------------------------
// Load the document to parse.
//----------------------------------------------------------------------------

void ts::TextParser::loadDocument(const UStringList& lines)
{
    _lines.clear();
    _pos = Position(lines);
}

void ts::TextParser::loadDocument(const UString& text)
{
    text.toSubstituted(u"\r", UString()).split(_lines, u'\n', false);
    _pos = Position(_lines);
}

bool ts::TextParser::loadFile(const UString& fileName)
{
    // Load the file into the internal lines buffer.
    const bool ok = UString::Load(_lines, fileName);
    if (!ok) {
        _report.error(u"error reading file %s", {fileName});
    }

    // Initialize the parser on the internal lines buffer, including on file error (empty).
    _pos = Position(_lines);
    return ok;
}

bool ts::TextParser::loadStream(std::istream& strm)
{
    // Load the file into the internal lines buffer.
    const bool ok = UString::Load(_lines, strm);
    if (!ok) {
        _report.error(u"error reading input document");
    }

    // Initialize the parser on the internal lines buffer, including on file error (empty).
    _pos = Position(_lines);
    return ok;
}


//----------------------------------------------------------------------------
// Restore a previous position in the document.
//----------------------------------------------------------------------------

bool ts::TextParser::seek(const Position& pos)
{
    // Check that we are still on the same document. This is a minimum fool-proof check.
    // But there is no guarantee that pos.curLine is still valid.
    if (pos._lines == _pos._lines) {
        _pos = pos;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Check or change position.
//----------------------------------------------------------------------------

bool ts::TextParser::eof() const
{
    return _pos._curLine == _pos._lines->end();
}

bool ts::TextParser::eol() const
{
    return _pos._curLine == _pos._lines->end() || _pos._curIndex >= _pos._curLine->length();
}

void ts::TextParser::rewind()
{
    _pos = Position(_lines);
}


//----------------------------------------------------------------------------
// Skip all whitespaces.
//----------------------------------------------------------------------------

bool ts::TextParser::skipWhiteSpace()
{
    while (_pos._curLine != _pos._lines->end()) {
        // Skip spaces in current line.
        while (_pos._curIndex < _pos._curLine->length() && IsSpace((*_pos._curLine)[_pos._curIndex])) {
            _pos._curIndex++;
        }
        // Stop if not at end of line (non-space character found).
        if (_pos._curIndex < _pos._curLine->length()) {
            return true;
        }
        // Move to next line.
        _pos._curLine++;
        _pos._curLineNumber++;
        _pos._curIndex = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Skip to next line.
//----------------------------------------------------------------------------

bool ts::TextParser::skipLine()
{
    while (_pos._curLine != _pos._lines->end()) {
        _pos._curLine++;
        _pos._curLineNumber++;
        _pos._curIndex = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if the current position in the document matches a string.
//----------------------------------------------------------------------------

bool ts::TextParser::match(const UString& str, bool skipIfMatch, CaseSensitivity cs)
{
    if (_pos._curLine == _pos._lines->end()) {
        // Already at end of document.
        return false;
    }

    size_t lineIndex = _pos._curIndex;
    size_t strIndex = 0;
    while (strIndex < str.length()) {
        if (lineIndex >= _pos._curLine->length() || !Match(str[strIndex++], (*_pos._curLine)[lineIndex++], cs)) {
            // str does not match
            return false;
        }
    }

    if (skipIfMatch) {
        _pos._curIndex = lineIndex;
    }
    return true;
}


//----------------------------------------------------------------------------
// Check and parse names.
//----------------------------------------------------------------------------

bool ts::TextParser::isNameStartChar(UChar c) const
{
    return IsAlpha(c) || c == u':' || c == u'_';
}

bool ts::TextParser::isNameChar(UChar c) const
{
    return isNameStartChar(c) || IsDigit(c) || c == u'.' || c == u'-';
}

bool ts::TextParser::isAtNameStart() const
{
    return _pos._curLine != _pos._lines->end() &&
        _pos._curIndex < _pos._curLine->length() &&
        isNameStartChar((*_pos._curLine)[_pos._curIndex]);
}


//----------------------------------------------------------------------------
// Parse a tag name.
//----------------------------------------------------------------------------

bool ts::TextParser::parseName(UString& name)
{
    name.clear();

    // Check that the next character is valid to start a name.
    if (!isAtNameStart()) {
        return false;
    }

    // Get the name.
    UChar c;
    while (_pos._curIndex < _pos._curLine->length() && isNameChar(c = (*_pos._curLine)[_pos._curIndex])) {
        name.append(c);
        _pos._curIndex++;
    }
    return true;
}


//----------------------------------------------------------------------------
// Parse text up to a given token.
//----------------------------------------------------------------------------

bool ts::TextParser::parseText(UString& result, const UString endToken, bool skipIfMatch, bool translateEntities)
{
    result.clear();
    bool found = false;

    // Loop on all lines until the end token is found.
    while (!found && _pos._curLine != _pos._lines->end()) {

        // Search for the end token in current line.
        const size_t end = _pos._curLine->find(endToken, _pos._curIndex);
        if (end == UString::NPOS) {
            // End token not found, include the complete end of line.
            result.append(*_pos._curLine, _pos._curIndex);
            result.append(LINE_FEED);
            _pos._curLine++;
            _pos._curLineNumber++;
            _pos._curIndex = 0;
        }
        else {
            // Found end token, stop here.
            result.append(*_pos._curLine, _pos._curIndex, end - _pos._curIndex);
            _pos._curIndex = skipIfMatch ? end + endToken.length() : end;
            found = true;
        }
    }

    // Translate HTML entities in the result if required.
    if (translateEntities) {
        result.convertFromHTML();
    }

    return found;
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTextParser.h"


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

ts::TextParser::~TextParser()
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

bool ts::TextParser::loadFile(const fs::path& fileName)
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
// Save the document to parse to a text file.
//----------------------------------------------------------------------------

bool ts::TextParser::saveFile(const fs::path& fileName)
{
    return UString::Save(_lines, fileName);
}

bool ts::TextParser::saveStream(std::ostream& strm)
{
    return UString::Save(_lines, strm);
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

bool ts::TextParser::isXMLNameStartChar(UChar c) const
{
    return IsAlpha(c) || c == u':' || c == u'_';
}

bool ts::TextParser::isXMLNameChar(UChar c) const
{
    return isXMLNameStartChar(c) || IsDigit(c) || c == u'.' || c == u'-';
}

bool ts::TextParser::isAtXMLNameStart() const
{
    return _pos._curLine != _pos._lines->end() &&
        _pos._curIndex < _pos._curLine->length() &&
        isXMLNameStartChar((*_pos._curLine)[_pos._curIndex]);
}


//----------------------------------------------------------------------------
// Parse an XML name.
//----------------------------------------------------------------------------

bool ts::TextParser::parseXMLName(UString& name)
{
    name.clear();

    // Check that the next character is valid to start a name.
    if (!isAtXMLNameStart()) {
        return false;
    }

    // Get the name.
    UChar c;
    while (_pos._curIndex < _pos._curLine->length() && isXMLNameChar(c = (*_pos._curLine)[_pos._curIndex])) {
        name.append(c);
        _pos._curIndex++;
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if the parser is at the start of a number (digit or minus sign).
//----------------------------------------------------------------------------

bool ts::TextParser::isAtNumberStart() const
{
    UChar c = CHAR_NULL;
    return _pos._curLine != _pos._lines->end() &&
        _pos._curIndex < _pos._curLine->length() &&
        (IsDigit(c = (*_pos._curLine)[_pos._curIndex]) || c == u'-' || c == u'+');
}


//----------------------------------------------------------------------------
// Parse a numeric literal.
//----------------------------------------------------------------------------

bool ts::TextParser::parseNumericLiteral(UString& str, bool allowHexa, bool allowFloat)
{
    str.clear();

    // Eliminate end of file or line.
    if (_pos._curLine == _pos._lines->end() || _pos._curIndex >= _pos._curLine->length()) {
        return false;
    }

    const UString& line(*_pos._curLine);
    const size_t end = line.length();
    size_t index = _pos._curIndex;

    // Skip optional sign.
    if (line[index] == u'-' || line[index] == u'+') {
        ++index;
    }

    // Detect number start.
    if (index >= end || !IsDigit(line[index])) {
        return false;
    }

    // Detect hexadecimal literal, skip integral part.
    if (index + 2 < end && line[index] == u'0' && (line[index+1] == u'x' || line[index+1] == u'X') && IsHexa(line[index+2])) {
        // Detected hexadecimal prefix, skip it.
        index += 3;
        // Reject if hexa not allowed by caller.
        if (!allowHexa) {
            return false;
        }
        // Reject floating point format with hexa.
        allowFloat = false;
        // Skip all hexa digits.
        while (index < end && IsHexa(line[index])) {
            ++index;
        }
    }
    else {
        // Skip decimal integral part.
        while (index < end && IsDigit(line[index])) {
            ++index;
        }
    }

    // Skip additional floating point representation.
    if (allowFloat) {
        if (index < end && line[index] == u'.') {
            ++index;
            while (index < end && IsDigit(line[index])) {
                ++index;
            }
        }
        if (index < end && (line[index] == u'e' || line[index] == u'E')) {
            ++index;
            if (index < end && (line[index] == u'+' || line[index] == u'-')) {
                ++index;
            }
            while (index < end && IsDigit(line[index])) {
                ++index;
            }
        }
    }

    // Reached end of numeric literal. Validate next character.
    if (index < end && (line[index] == u'.' || line[index] == u'_' || IsAlpha(line[index]))) {
        return false;
    }
    else {
        // Now we have a valid numeric literal.
        str = _pos._curLine->substr(_pos._curIndex, index - _pos._curIndex);
        _pos._curIndex = index;
        return true;
    }
}


//----------------------------------------------------------------------------
// Parse a string literal.
//----------------------------------------------------------------------------

bool ts::TextParser::parseStringLiteral(UString& str, UChar requiredQuote)
{
    str.clear();

    // Check that we are at the beginning of something.
    if (_pos._curLine == _pos._lines->end() || _pos._curIndex >= _pos._curLine->length()) {
        // At eol or eof.
        return false;
    }

    const UString& line(*_pos._curLine);
    const size_t end = line.length();
    size_t index = _pos._curIndex;

    // Validate the type of quote.
    const UChar quote = line[index++];
    if (requiredQuote == u'\'' && quote != u'\'') {
        return false;
    }
    if (requiredQuote == u'"' && quote != u'"') {
        return false;
    }
    if (quote != u'\'' && quote != u'"') {
        return false;
    }

    // Now parse all characters in the string.
    UChar c = CHAR_NULL;
    while (index < end && (c = line[index]) != quote) {
        index++;
        if (c == u'\\') {
            index++; // skip character after backslash.
        }
    }
    if (index >= end) {
        // Reached eol without finding the closing quote.
        return false;
    }
    else {
        // Now we have a string literal.
        str = line.substr(_pos._curIndex, index + 1 - _pos._curIndex);
        _pos._curIndex = index + 1;
        return true;
    }
}


//----------------------------------------------------------------------------
// Parse a JSON string literal.
//----------------------------------------------------------------------------

bool ts::TextParser::parseJSONStringLiteral(UString& str)
{
    // JSON strings always start with a double quote.
    if (parseStringLiteral(str, u'"')) {
        assert(str.length() >= 2);
        assert(str.front() == str.back());
        str.erase(0, 1);
        str.pop_back();
        str.convertFromJSON();
        return true;
    }
    else {
        return false;
    }
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
        if (end == NPOS) {
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

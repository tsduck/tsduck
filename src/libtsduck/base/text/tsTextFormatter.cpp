//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTextFormatter.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TextFormatter::TextFormatter(Report& report) :
    AbstractOutputStream(),
    _report(report),
    _out(&_outFile)  // _out is never null, points by default to a closed file (discard output)
{
}

ts::TextFormatter::~TextFormatter()
{
    close();
}


//----------------------------------------------------------------------------
// Set output to an open text stream.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::setStream(std::ostream& strm)
{
    close();
    _out = &strm;
    return *this;
}


//----------------------------------------------------------------------------
// Set output to a text file.
//----------------------------------------------------------------------------

bool ts::TextFormatter::setFile(const fs::path& fileName)
{
    close();
    _report.debug(u"creating file %s", {fileName});
    _outFile.open(fileName, std::ios::out);
    if (!_outFile) {
        _report.error(u"cannot create file %s", {fileName});
        return false;
    }
    else {
        _out = &_outFile;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set output to an internal string buffer.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::setString()
{
    close();
    _out = &_outString;
    return *this;
}


//----------------------------------------------------------------------------
// Retrieve the current contentn of the internal string buffer.
//----------------------------------------------------------------------------

bool ts::TextFormatter::getString(UString& str)
{
    if (_out != &_outString) {
        // Output is not set to string.
        str.clear();
        return false;
    }
    else {
        // Get internal buffer, do not reset it.
        flush();
        str.assignFromUTF8(_outString.str());
        // Cleanup end of lines.
        str.substitute(UString(1, CARRIAGE_RETURN), UString());
        return true;
    }
}

ts::UString ts::TextFormatter::toString()
{
    UString str;
    getString(str);
    return str;
}


//----------------------------------------------------------------------------
// Check if the Output is open to some output.
//----------------------------------------------------------------------------

bool ts::TextFormatter::isOpen() const
{
    return _out != &_outFile || _outFile.is_open();
}


//----------------------------------------------------------------------------
// Close the current output.
//----------------------------------------------------------------------------

void ts::TextFormatter::close()
{
    // Flush buffered characters.
    flush();

    // Close resources.
    if (_out == &_outString) {
        // Output is set to string. Reset internal buffer.
        _outString.str(std::string());
    }
    if (_outFile.is_open()) {
        _outFile.close();
    }

    // Set output to a closed file. Thus, _out is never null, it is safe to
    // output to *_out, but output is discarded (closed file).
    _out = &_outFile;

    // Reset margin.
    _column = 0;
    _afterSpace = false;
    _curMargin = _margin;
}


//----------------------------------------------------------------------------
// Set the margin size for outer-most elements.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::setMarginSize(size_t margin)
{
    // Try to adjust current margin by the same amount.
    if (margin > _margin) {
        _curMargin += margin - _margin;
    }
    else if (margin < _margin) {
        _curMargin -= std::min(_curMargin, _margin - margin);
    }

    // Set the new margin.
    _margin = margin;
    return *this;
}


//----------------------------------------------------------------------------
// Implementation of AbstractOutputStream
//----------------------------------------------------------------------------

bool ts::TextFormatter::writeStreamBuffer(const void* addr, size_t size)
{
    const char* const last = static_cast<const char*>(addr) + size;
    for (const char* p = static_cast<const char*>(addr); p < last; ++p) {
        if (*p == '\t') {
            // Tabulations are expanded as spaces.
            // Without formatting, a tabulation is just one space.
            do {
                *_out << ' ';
            } while (++_column % _tabSize != 0 && _formatting);
        }
        else if (*p == '\r' || *p == '\n') {
            // CR and LF indifferently move back to begining of current/next line.
            *_out << *p;
            _column = 0;
            _afterSpace = false;
        }
        else {
            *_out << *p;
            ++_column;
            _afterSpace = _afterSpace || *p != ' ';
        }
    }
    return !_out->fail();
}


//----------------------------------------------------------------------------
// Set the end-of-line mode.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::setEndOfLineMode(EndOfLineMode mode)
{
    if (mode != _eolMode) {
        // Flush to apply previous format to pending output.
        flush();
        // Then switch format.
        _eolMode = mode;
        _formatting = _eolMode != EndOfLineMode::SPACING && _eolMode != EndOfLineMode::NONE;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Insert an end-of-line, according to the current end-of-line mode.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::endl()
{
    // Flush pending data to _out.
    flush();

    // Different types of end-of-line.
    switch (_eolMode) {
        case EndOfLineMode::NATIVE:
            *_out << std::endl;
            _column = 0;
            _afterSpace = false;
            break;
        case EndOfLineMode::CR:
            *_out << CARRIAGE_RETURN;
            _column = 0;
            _afterSpace = false;
            break;
        case EndOfLineMode::LF:
            *_out << LINE_FEED;
            _column = 0;
            _afterSpace = false;
            break;
        case EndOfLineMode::CRLF:
            *_out << CARRIAGE_RETURN << LINE_FEED;
            _column = 0;
            _afterSpace = false;
            break;
        case EndOfLineMode::SPACING:
            *_out << SPACE;
            _column++;
            _afterSpace = false;
            break;
        case EndOfLineMode::NONE:
        default:
            break;
    }

    return *this;
}

//----------------------------------------------------------------------------
// Insert all necessary new-lines and spaces to move to the current margin.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::margin()
{
    // Do nothing if no line breaks are produced (there is no margin).
    if (_formatting) {

        // Flush pending data to _out.
        flush();

        // New line if we are farther than the margin. Also new line when we are no longer
        // in the margin ("after space") even if we do not exceed the margin size.
        if (_column > _curMargin || _afterSpace) {
            endl();
        }

        // Move to the margin.
        *_out << std::string(_curMargin - _column, ' ');
        _column = _curMargin;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Insert all necessary new-lines and spaces to move to a given column.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::column(size_t col)
{
    // Do nothing if no line breaks are produced (there is no column).
    if (_formatting) {

        // Flush pending output to get valid _column and _afterSpace.
        flush();

        // New line if we are farther than the target col.
        if (_column > col) {
            endl();
        }

        // Move to the specified column.
        *_out << std::string(col - _column, ' ');
        _column = col;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Output spaces on the stream.
//----------------------------------------------------------------------------

ts::TextFormatter& ts::TextFormatter::spaces(size_t count)
{
    flush();
    *_out << std::string(count, ' ');
    _column += count;
    return *this;
}


//----------------------------------------------------------------------------
// I/O manipulators.
//----------------------------------------------------------------------------

ts::IOManipulatorProxy<ts::TextFormatter, size_t> ts::margin(size_t size)
{
    return IOManipulatorProxy<TextFormatter, size_t>(&TextFormatter::setMarginSize, size);
}

ts::IOManipulatorProxy<ts::TextFormatter, size_t> ts::spaces(size_t count)
{
    return IOManipulatorProxy<TextFormatter, size_t>(&TextFormatter::spaces, count);
}

ts::IOManipulatorProxy<ts::TextFormatter, size_t> ts::column(size_t col)
{
    return IOManipulatorProxy<TextFormatter, size_t>(&TextFormatter::column, col);
}

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

#include "tsxmlOutput.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::xml::Output::Output(Report& report) :
    _report(_report),
    _outFile(),
    _outString(),
    _out(&_outFile), // _out is never null, points by default to a closed file (discard output)
    _margin(0),
    _indent(2),
    _compact(false),
    _curMargin(_margin)
{
}

ts::xml::Output::~Output()
{
    close();
}


//----------------------------------------------------------------------------
// Set output to an open text stream.
//----------------------------------------------------------------------------

ts::xml::Output& ts::xml::Output::setStream(std::ostream& strm)
{
    close();
    _out = &strm;
    return *this;
}


//----------------------------------------------------------------------------
// Set output to a text file.
//----------------------------------------------------------------------------

bool ts::xml::Output::setFile(const UString& fileName)
{
    close();
    _outFile.open(fileName.toUTF8().c_str(), std::ios::out);
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

ts::xml::Output& ts::xml::Output::setString()
{
    close();
    _out = &_outString;
    return *this;
}


//----------------------------------------------------------------------------
// Retrieve the current contentn of the internal string buffer. 
//----------------------------------------------------------------------------

bool ts::xml::Output::getString(UString& str)
{
    if (_out != &_outString) {
        // Output is not set to string.
        str.clear();
        return false;
    }
    else {
        // Get internal buffer, do not reset it.
        str.assignFromUTF8(_outString.str());
        return true;
    }
}

ts::UString ts::xml::Output::toString()
{
    return _out != &_outString ? UString() : UString::FromUTF8(_outString.str());
}


//----------------------------------------------------------------------------
// Check if the Output is open to some output.
//----------------------------------------------------------------------------

bool ts::xml::Output::isOpen() const
{
    return _out != &_outFile || _outFile.is_open();
}


//----------------------------------------------------------------------------
// Close the current output.
//----------------------------------------------------------------------------

void ts::xml::Output::close()
{
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
    _curMargin = _margin;
}


//----------------------------------------------------------------------------
// Output new lines and spaces on a stream if not in compact mode.
//----------------------------------------------------------------------------

std::ostream& ts::xml::Output::newLine() const
{
    if (!_compact) {
        *_out << std::endl;
    }
    return *_out;
}

std::ostream& ts::xml::Output::spaces(size_t count) const
{
    if (!_compact && count > 0) {
        *_out << std::string(count, ' ');
    }
    return *_out;
}

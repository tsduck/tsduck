//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsSubRipGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::SubRipGenerator::SubRipGenerator(const UString& fileName, Report& report) :
    _outputStream(),
    _stream(nullptr),
    _frameCount(0)
{
    if (!fileName.empty()) {
        open(fileName, report);
    }
}

ts::SubRipGenerator::SubRipGenerator(std::ostream* stream) :
    _outputStream(),
    _stream(nullptr),
    _frameCount(0)
{
    setStream(stream);
}

ts::SubRipGenerator::~SubRipGenerator()
{
    close();
}


//----------------------------------------------------------------------------
// Open or re-open the generator.
//----------------------------------------------------------------------------

bool ts::SubRipGenerator::open(const UString& fileName, Report& report)
{
    close();

    _outputStream.open(fileName.toUTF8().c_str(), std::ios::out);

    if (!_outputStream) {
        report.error(u"error creating file %s", {fileName});
        return false;
    }
    else {
        _stream = &_outputStream;
        return true;
    }
}

bool ts::SubRipGenerator::setStream(std::ostream* stream)
{
    close();
    _stream = stream;
    return _stream != nullptr;
}


//----------------------------------------------------------------------------
// Close the generator.
//----------------------------------------------------------------------------

void ts::SubRipGenerator::close()
{
    if (_stream != nullptr) {
        _stream->flush();
        _stream = nullptr;
    }
    if (_outputStream.is_open()) {
        _outputStream.close();
    }
    _frameCount = 0;
}


//----------------------------------------------------------------------------
// Add a multi-lines subtitle frame.
//----------------------------------------------------------------------------

void ts::SubRipGenerator::addFrame(MilliSecond showTimestamp, MilliSecond hideTimestamp, const UString& line)
{
    UStringList list;
    list.push_back(line);
    addFrame(showTimestamp, hideTimestamp, list);
}

void ts::SubRipGenerator::addFrame(MilliSecond showTimestamp, MilliSecond hideTimestamp, const UStringList& lines)
{
    // Empty lines are illegal in SRT. Make sure we have at least one non-empty line.
    bool notEmpty = false;
    for (const auto& it : lines) {
        if (!it.empty()) {
            notEmpty = true;
            break;
        }
    }

    // Generate the frame only when it is possible to do so.
    if (notEmpty && _stream != nullptr) {
        // First line: Frame count, starting at 1.
        // Second line: Start and end timestamps.
        *_stream << ++_frameCount << std::endl
                 << FormatDuration(showTimestamp, hideTimestamp) << std::endl;

        // Subsequent lines: Subtitle text.
        for (const auto& it : lines) {
            // Empty lines are illegal in SRT, skip them.
            if (!it.empty()) {
                *_stream << it << std::endl;
            }
        }
        // Trailing empty line to mark the end of frame.
        *_stream << std::endl;
    }
}


//----------------------------------------------------------------------------
// Format SRT times.
//----------------------------------------------------------------------------

ts::UString ts::SubRipGenerator::FormatTime(MilliSecond timestamp)
{
    // Tims stamp is in milliseconds.
    const int h = int(timestamp / 3600000);
    const int m = int(timestamp / 60000 - 60 * h);
    const int s = int(timestamp / 1000 - 3600 * h - 60 * m);
    const int u = int(timestamp - 3600000 * h - 60000 * m - 1000 * s);
    return UString::Format(u"%02d:%02d:%02d,%03d", {h, m, s, u});
}

ts::UString ts::SubRipGenerator::FormatDuration(MilliSecond showTimestamp, MilliSecond hideTimestamp)
{
    return FormatTime(showTimestamp) + u" --> " + FormatTime(hideTimestamp);
}

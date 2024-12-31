//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSubRipGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::SubRipGenerator::SubRipGenerator(const fs::path& fileName, Report& report)
{
    if (!fileName.empty()) {
        open(fileName, report);
    }
}

ts::SubRipGenerator::SubRipGenerator(std::ostream* stream)
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

bool ts::SubRipGenerator::open(const fs::path& fileName, Report& report)
{
    close();

    _outputStream.open(fileName, std::ios::out);

    if (!_outputStream) {
        report.error(u"error creating file %s", fileName);
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

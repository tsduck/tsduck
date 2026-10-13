//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSubRipGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::SubRipGenerator::SubRipGenerator(const fs::path& file_name, Report& report)
{
    if (!file_name.empty()) {
        open(file_name, report);
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

bool ts::SubRipGenerator::open(const fs::path& file_name, Report& report)
{
    close();

    _output_stream.open(file_name, std::ios::out);

    if (!_output_stream) {
        report.error(u"error creating file %s", file_name);
        return false;
    }
    else {
        _stream = &_output_stream;
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
    if (_output_stream.is_open()) {
        _output_stream.close();
    }
    _frame_count = 0;
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOutputRedirector.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor:
//----------------------------------------------------------------------------

ts::OutputRedirector::OutputRedirector(const fs::path& name, Args& args, std::ostream& stream, std::ios::openmode mode) :
    _stream(stream)
{
    // The nme "-" means standard output.
    if (!name.empty() && name != u"-") {
        _file.open(name, mode);
        if (_file) {
            _previous = _stream.rdbuf(_file.rdbuf());
        }
        else {
            args.error(u"cannot open file %s", {name});
            args.exitOnError();
        }
    }
    else if (&stream == &std::cout && (mode | std::ios::binary) == mode) {
        // Try to put standard output in binary mode
        SetBinaryModeStdout(args);
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::OutputRedirector::~OutputRedirector()
{
    if (_previous != nullptr) {
        _stream.rdbuf(_previous);
        _previous = nullptr;
    }
    if (_file.is_open()) {
        _file.close();
    }
}

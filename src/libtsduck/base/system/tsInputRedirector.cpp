//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInputRedirector.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor:
//----------------------------------------------------------------------------

ts::InputRedirector::InputRedirector(const fs::path& name, Args& args, std::istream& stream, std::ios::openmode mode) :
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
    else if (&stream == &std::cin && (mode | std::ios::binary) == mode) {
        // Try to put standard input in binary mode
        SetBinaryModeStdin(args);
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::InputRedirector::~InputRedirector()
{
    if (_previous != nullptr) {
        _stream.rdbuf(_previous);
        _previous = nullptr;
    }
    if (_file.is_open()) {
        _file.close();
    }
}

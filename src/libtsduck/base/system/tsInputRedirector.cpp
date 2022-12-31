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

#include "tsInputRedirector.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor:
//----------------------------------------------------------------------------

ts::InputRedirector::InputRedirector(const UString& name,
                                     Args& args,
                                     std::istream& stream,
                                     std::ios::openmode mode) :
    _stream(stream),
    _previous(nullptr),
    _file()
{
    // The nme "-" means standard output.
    if (!name.empty() && name != u"-") {
        _file.open(name.toUTF8().c_str(), mode);
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

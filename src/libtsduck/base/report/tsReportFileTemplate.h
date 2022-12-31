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

#pragma once
#include "tsGuardMutex.h"
#include <iostream>


//----------------------------------------------------------------------------
// Constructor using a named file.
//----------------------------------------------------------------------------

template <class MUTEX>
ts::ReportFile<MUTEX>::ReportFile(const UString& file_name, bool append, int max_severity) :
    Report(max_severity),
    _mutex(),
    _file_name(file_name.toUTF8()),
    _named_file(_file_name.c_str(), append ? (std::ios::out | std::ios::app) : std::ios::out),
    _file(_named_file)
{
    // Process error when creating the file
    if (!_named_file) {
        std::cerr << "Fatal error creating log file " << file_name << std::endl;
    }
}


//----------------------------------------------------------------------------
// Constructor using an open file stream.
//----------------------------------------------------------------------------

template <class MUTEX>
ts::ReportFile<MUTEX>::ReportFile(std::ostream& stream, int max_severity) :
    Report(max_severity),
    _mutex(),
    _file_name(),
    _named_file(),
    _file(stream)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <class MUTEX>
ts::ReportFile<MUTEX>::~ReportFile()
{
    GuardMutex lock(_mutex);

    // Close the file if it was explicitly open by constructor
    if (_named_file.is_open()) {
        _named_file.close();
    }
}
TS_POP_WARNING()


//----------------------------------------------------------------------------
// Message processing handler, must be implemented in actual classes.
//----------------------------------------------------------------------------

template <class MUTEX>
void ts::ReportFile<MUTEX>::writeLog(int severity, const UString& message)
{
    GuardMutex lock(_mutex);
    _file << Severity::Header(severity) << message << std::endl;
}

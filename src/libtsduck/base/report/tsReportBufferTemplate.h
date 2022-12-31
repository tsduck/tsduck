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


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

template <class MUTEX>
ts::ReportBuffer<MUTEX>::ReportBuffer(int max_severity) :
    Report(max_severity),
    _mutex(),
    _buffer()
{
}


//----------------------------------------------------------------------------
// Reset the content of the internal buffer.
//----------------------------------------------------------------------------

template <class MUTEX>
void ts::ReportBuffer<MUTEX>::resetMessages()
{
    GuardMutex lock(_mutex);
    _buffer.clear();
}


//----------------------------------------------------------------------------
// Check if the content of the internal buffer is empty.
//----------------------------------------------------------------------------

template <class MUTEX>
bool ts::ReportBuffer<MUTEX>::emptyMessages() const
{
    GuardMutex lock(_mutex);
    return _buffer.empty();
}


//----------------------------------------------------------------------------
// Get the content of the internal buffer.
//----------------------------------------------------------------------------

template <class MUTEX>
ts::UString ts::ReportBuffer<MUTEX>::getMessages() const
{
    GuardMutex lock(_mutex);
    return _buffer;
}


//----------------------------------------------------------------------------
// Message processing handler, add the message in the buffer.
//----------------------------------------------------------------------------

template <class MUTEX>
void ts::ReportBuffer<MUTEX>::writeLog(int severity, const UString& message)
{
    GuardMutex lock(_mutex);
    if (!_buffer.empty()) {
        _buffer.append(u'\n');
    }
    _buffer.append(Severity::Header(severity));
    _buffer.append(message);
}

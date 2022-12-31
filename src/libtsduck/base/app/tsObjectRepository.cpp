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

#include "tsObjectRepository.h"
#include "tsGuardMutex.h"

TS_DEFINE_SINGLETON(ts::ObjectRepository);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ObjectRepository::ObjectRepository() :
    _mutex(),
    _repository()
{
}


//----------------------------------------------------------------------------
// Store a safe pointer to an Object in the repository.
//----------------------------------------------------------------------------

ts::ObjectPtr ts::ObjectRepository::store(const UString &name, const ObjectPtr &value)
{
    GuardMutex lock(_mutex);
    const ObjectPtr previous = _repository[name];
    if (value.isNull()) {
        _repository.erase(name);
    }
    else {
        _repository[name] = value;
    }
    return previous;
}


//----------------------------------------------------------------------------
// Get the safe pointer to an Object in the repository
//----------------------------------------------------------------------------

ts::ObjectPtr ts::ObjectRepository::retrieve(const UString &name)
{
    GuardMutex lock(_mutex);
    const auto pos = _repository.find(name);
    return pos != _repository.end() ? pos->second : ObjectPtr();
}


//----------------------------------------------------------------------------
// Erase from the repository the value of a name.
//----------------------------------------------------------------------------

void ts::ObjectRepository::erase(const UString& name)
{
    GuardMutex lock(_mutex);
    _repository.erase(name);
}

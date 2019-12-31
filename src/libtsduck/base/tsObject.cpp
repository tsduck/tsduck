//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsObject.h"
TSDUCK_SOURCE;

// Static thread-safe repository of Object
std::map <ts::UString, ts::ObjectPtr> ts::Object::_repository;
ts::Mutex ts::Object::_repository_mutex;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Object::~Object()
{
}


//----------------------------------------------------------------------------
// Store a safe pointer to an Object in a static thread-safe repository.
//----------------------------------------------------------------------------

ts::ObjectPtr ts::Object::StoreInRepository(const UString& name, const ObjectPtr& value)
{
    Guard lock(_repository_mutex);
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
// Get the safe pointer to an Object in the static thread-safe repository
//----------------------------------------------------------------------------

ts::ObjectPtr ts::Object::RetrieveFromRepository(const UString& name)
{
    Guard lock(_repository_mutex);
    const std::map <UString, ObjectPtr>::const_iterator pos = _repository.find(name);
    return pos != _repository.end() ? pos->second : ObjectPtr();
}


//----------------------------------------------------------------------------
// Erase from the static thread-safe repository the value which is associated to
// the specified name.
//----------------------------------------------------------------------------

void ts::Object::EraseFromRepository(const UString& name)
{
    Guard lock(_repository_mutex);
    _repository.erase(name);
}

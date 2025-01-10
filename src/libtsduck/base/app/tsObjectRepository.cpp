//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsObjectRepository.h"

TS_DEFINE_SINGLETON(ts::ObjectRepository);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ObjectRepository::ObjectRepository()
{
}


//----------------------------------------------------------------------------
// Store a safe pointer to an Object in the repository.
//----------------------------------------------------------------------------

ts::ObjectPtr ts::ObjectRepository::store(const UString& name, const ObjectPtr& value)
{
    // Write lock (exclusive).
    std::lock_guard<std::shared_mutex> lock(_mutex);

    const ObjectPtr previous = _repository[name];
    if (value == nullptr) {
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

ts::ObjectPtr ts::ObjectRepository::retrieve(const UString& name) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    const auto pos = _repository.find(name);
    return pos != _repository.end() ? pos->second : ObjectPtr();
}


//----------------------------------------------------------------------------
// Erase from the repository the value of a name.
//----------------------------------------------------------------------------

void ts::ObjectRepository::erase(const UString& name)
{
    // Write lock (exclusive).
    std::lock_guard<std::shared_mutex> lock(_mutex);

    _repository.erase(name);
}

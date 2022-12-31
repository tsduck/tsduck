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

#include "tsTablesLoggerFilterRepository.h"

TS_DEFINE_SINGLETON(ts::TablesLoggerFilterRepository);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TablesLoggerFilterRepository::TablesLoggerFilterRepository() :
    _factories()
{
}


//----------------------------------------------------------------------------
// This inner class constructor registers a section filter factory.
//----------------------------------------------------------------------------

ts::TablesLoggerFilterRepository::Register::Register(FilterFactory factory)
{
    if (factory != nullptr) {
        TablesLoggerFilterRepository::Instance()->_factories.push_back(factory);
    }
}


//----------------------------------------------------------------------------
// Create an instance of all registered section filters.
//----------------------------------------------------------------------------

void ts::TablesLoggerFilterRepository::createFilters(TablesLoggerFilterVector& filters) const
{
    // Pre-reserve memory.
    filters.clear();
    filters.reserve(_factories.size());

    // Allocate an instance of each registered class.
    // Encapsulate all allocated object into a safe pointer.
    for (size_t i = 0; i < _factories.size(); ++i) {
        if (_factories[i] != nullptr) {
            TablesLoggerFilterPtr ptr(_factories[i]());
            if (!ptr.isNull()) {
                filters.push_back(ptr);
            }
        }
    }
}

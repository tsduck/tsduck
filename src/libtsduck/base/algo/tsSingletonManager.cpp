//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSingletonManager.h"

ts::SingletonManager* volatile ts::SingletonManager::_instance = nullptr;

// Instance. May be subject to race condition if threads are created
// before creating the first singleton.

ts::SingletonManager* ts::SingletonManager::Instance()
{
    if (_instance == nullptr) {
        _instance = new SingletonManager;
    }
    return _instance;
}

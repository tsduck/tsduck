//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveWorkerInterface.h"
#include "tsSysUtils.h"

ts::ReactiveWorkerInterface::~ReactiveWorkerInterface() {}
int ts::ReactiveWorkerInterface::executeWork(ReactiveWorkerPool&, const ObjectPtr&) { return SYS_SUCCESS; }

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveWorkerHandlerInterface.h"

ts::ReactiveWorkerHandlerInterface::~ReactiveWorkerHandlerInterface() {}
void ts::ReactiveWorkerHandlerInterface::handleWorkerCompletion(ReactiveWorkerPool&, int, const ObjectPtr&) {}

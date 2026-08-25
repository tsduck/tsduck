//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveWebHandlerInterface.h"

ts::ReactiveWebHandlerInterface::~ReactiveWebHandlerInterface() {}
void ts::ReactiveWebHandlerInterface::handleWebOpen(ts::ReactiveWebRequest&, int, const ObjectPtr&) {}
void ts::ReactiveWebHandlerInterface::handleWebReceive(ts::ReactiveWebRequest&, int, const ObjectPtr&) {}
void ts::ReactiveWebHandlerInterface::handleWebClosed(ts::ReactiveWebRequest&, int, const ObjectPtr&) {}

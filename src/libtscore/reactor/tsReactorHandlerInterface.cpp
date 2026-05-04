//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactorHandlerInterface.h"


ts::ReactorHandlerInterface::~ReactorHandlerInterface() {}
void ts::ReactorHandlerInterface::handleTimer(Reactor&, EventId) {}
void ts::ReactorHandlerInterface::handleUserEvent(Reactor&, EventId) {}
void ts::ReactorHandlerInterface::handleReadReady(Reactor&, EventId, int) {}
void ts::ReactorHandlerInterface::handleWriteReady(Reactor&, EventId, int) {}
void ts::ReactorHandlerInterface::handleAsynchronousIO(Reactor&, EventId, NonBlockingDevice::IOSB&, size_t, int) {}

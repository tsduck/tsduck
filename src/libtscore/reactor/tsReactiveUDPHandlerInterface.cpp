//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveUDPHandlerInterface.h"


ts::ReactiveUDPHandlerInterface::~ReactiveUDPHandlerInterface() {}
void ts::ReactiveUDPHandlerInterface::handleUDPSend(ReactiveUDPSocket&, const void*, size_t, const IPSocketAddress&, int, const ObjectPtr&) {}
void ts::ReactiveUDPHandlerInterface::handleUDPReceive(ReactiveUDPSocket&, const ByteBlockPtr&, const IPSocketAddress&, const IPSocketAddress&,
                                                       cn::microseconds, UDPSocket::TimeStampType, int, const ObjectPtr&) {}
void ts::ReactiveUDPHandlerInterface::handleUDPClosed(ReactiveUDPSocket&, const ObjectPtr&) {}

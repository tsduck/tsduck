//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTCPConnectionHandlerInterface.h"

ts::ReactiveTCPConnectionHandlerInterface::~ReactiveTCPConnectionHandlerInterface() {}
void ts::ReactiveTCPConnectionHandlerInterface::handleTCPConnected(ReactiveTCPConnection&, int, const ObjectPtr&) {}
void ts::ReactiveTCPConnectionHandlerInterface::handleTCPAccepted(ReactiveTCPServer&, ReactiveTCPConnection&, int, const ObjectPtr&) {}
void ts::ReactiveTCPConnectionHandlerInterface::handleTCPSend(ReactiveTCPConnection&, size_t, int, const ObjectPtr&) {}
void ts::ReactiveTCPConnectionHandlerInterface::handleTCPReceive(ReactiveTCPConnection&, const ByteBlock&, ReactiveTCPInputControl&, int, const ObjectPtr&) {}
void ts::ReactiveTCPConnectionHandlerInterface::handleTCPClosed(ReactiveTCPConnection&, const ObjectPtr&) {}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTCPServerHandlerInterface.h"

ts::ReactiveTCPServerHandlerInterface::~ReactiveTCPServerHandlerInterface() {}
void ts::ReactiveTCPServerHandlerInterface::handleTCPClientAccepted(ReactiveTCPServer&, ReactiveTCPConnection&, const IPSocketAddress&, int, const ObjectPtr&) {}
void ts::ReactiveTCPServerHandlerInterface::handleTCPServerClosed(ReactiveTCPServer&, const ObjectPtr&) {}

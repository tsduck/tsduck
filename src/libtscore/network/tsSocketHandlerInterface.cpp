//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSocketHandlerInterface.h"

ts::SocketHandlerInterface::~SocketHandlerInterface() {}
void ts::SocketHandlerInterface::handleSocketOpenStart(Socket&) {}
void ts::SocketHandlerInterface::handleSocketOpenComplete(Socket&, bool) {}
void ts::SocketHandlerInterface::handleSocketConnected(TCPConnection&) {}
void ts::SocketHandlerInterface::handleSocketDisconnected(TCPConnection&, bool) {}
void ts::SocketHandlerInterface::handleSocketCloseStart(Socket&, bool) {}
void ts::SocketHandlerInterface::handleSocketCloseComplete(Socket&, bool, bool) {}

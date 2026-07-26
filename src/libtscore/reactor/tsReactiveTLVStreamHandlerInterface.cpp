//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLVStreamHandlerInterface.h"

ts::ReactiveTLVStreamHandlerInterface::~ReactiveTLVStreamHandlerInterface() {}
void ts::ReactiveTLVStreamHandlerInterface::handleReceiveMessage(ReactiveTLVStream&, const tlv::MessagePtr&, int, const ObjectPtr&) {}
void ts::ReactiveTLVStreamHandlerInterface::handleSendMessage(ReactiveTLVStream&, const ByteBlockPtr&, int, const ObjectPtr&) {}

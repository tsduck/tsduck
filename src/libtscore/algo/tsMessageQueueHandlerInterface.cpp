//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMessageQueueHandlerInterface.h"


ts::MessageQueueHandlerInterface::~MessageQueueHandlerInterface() {}
void ts::MessageQueueHandlerInterface::handleMessageEnqueued(size_t queue_size) {}
void ts::MessageQueueHandlerInterface::handleMessageDequeued(size_t queue_size) {}

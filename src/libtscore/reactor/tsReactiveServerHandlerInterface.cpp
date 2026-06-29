//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveServerHandlerInterface.h"

ts::ReactiveServerHandlerInterface::~ReactiveServerHandlerInterface() {}
void ts::ReactiveServerHandlerInterface::handleServerExited(ReactiveServer& server, const ObjectPtr& user_data) {}

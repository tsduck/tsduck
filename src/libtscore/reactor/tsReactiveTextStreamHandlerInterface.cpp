//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTextStreamHandlerInterface.h"

ts::ReactiveTextStreamHandlerInterface::~ReactiveTextStreamHandlerInterface() {}
void ts::ReactiveTextStreamHandlerInterface::handleTextLine(ReactiveTextStream&, const UString&, int, const ObjectPtr&) {}
void ts::ReactiveTextStreamHandlerInterface::handleWriteText(ReactiveTextStream&, const std::string&, int, const ObjectPtr&) {}

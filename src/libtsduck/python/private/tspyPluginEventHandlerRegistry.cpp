//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: encapsulates PluginEventHandlerRegistry
//  intermediate class for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsPluginEventHandlerRegistry.h"


TSDUCKPY void tspyPluginEventHandlerRegister(void* tsp, ts::PluginEventHandlerInterface* handler, uint32_t event_code)
{
    ts::PluginEventHandlerRegistry* reg = reinterpret_cast<ts::PluginEventHandlerRegistry*>(tsp);
    if (reg != nullptr) {
        reg->registerEventHandler(handler, event_code);
    }
}

TSDUCKPY void tspyPluginEventHandlerRegisterInput(void* tsp, ts::PluginEventHandlerInterface* handler)
{
    ts::PluginEventHandlerRegistry* reg = reinterpret_cast<ts::PluginEventHandlerRegistry*>(tsp);
    if (reg != nullptr) {
        reg->registerEventHandler(handler, ts::PluginType::INPUT);
    }
}

TSDUCKPY void tspyPluginEventHandlerRegisterOutput(void* tsp, ts::PluginEventHandlerInterface* handler)
{
    ts::PluginEventHandlerRegistry* reg = reinterpret_cast<ts::PluginEventHandlerRegistry*>(tsp);
    if (reg != nullptr) {
        reg->registerEventHandler(handler, ts::PluginType::OUTPUT);
    }
}

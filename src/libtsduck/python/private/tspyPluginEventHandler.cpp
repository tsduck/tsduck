//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tspyPluginEventHandler.h"
#include "tsPluginEventData.h"
#include "tspy.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Python interface.
//----------------------------------------------------------------------------

TSDUCKPY void* tspyNewPyPluginEventHandler(ts::py::PluginEventHandler::PyCallback callback)
{
    return new ts::py::PluginEventHandler(callback);
}

TSDUCKPY void tspyDeletePyPluginEventHandler(void* obj)
{
    delete reinterpret_cast<ts::py::PluginEventHandler*>(obj);
}


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::py::PluginEventHandler::PluginEventHandler(PyCallback callback) :
    _callback(callback)
{
}

ts::py::PluginEventHandler::~PluginEventHandler()
{
}


//----------------------------------------------------------------------------
// Event handling method.
//----------------------------------------------------------------------------

void ts::py::PluginEventHandler::handlePluginEvent(const PluginEventContext& context)
{
    if (_callback != nullptr) {
        static const uint8_t dummy = 0;
        const PluginEventData* event_data = dynamic_cast<const PluginEventData*>(context.pluginData());
        const bool valid_data = event_data != nullptr && event_data->data() != nullptr;
        const UString name(context.pluginName());
        _callback(context.eventCode(),
                  name.data(),
                  name.size() * sizeof(UChar),
                  context.pluginIndex(),
                  context.pluginCount(),
                  size_t(context.bitrate()),
                  size_t(context.pluginPackets()),
                  size_t(context.totalPackets()),
                  valid_data ? event_data->data() : &dummy,
                  valid_data ? event_data->size() : 0);
    }
}

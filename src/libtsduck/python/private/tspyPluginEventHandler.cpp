//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tspyPluginEventHandler.h"
#include "tsPluginEventData.h"
#include "tspy.h"


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

// Update the content of a PluginEventData.
// Called from the Python callback.
TSDUCKPY void tspyPyPluginEventHandlerUpdateData(void* obj, void* data, size_t size)
{
    ts::PluginEventData* event_data = reinterpret_cast<ts::PluginEventData*>(obj);
    if (event_data != nullptr) {
        uint8_t* buffer = event_data->outputData();
        if (buffer != nullptr && data != nullptr && size <= event_data->maxSize()) {
            std::memcpy(buffer, data, size);
            event_data->updateSize(size);
        }
        else {
            event_data->setError(true);
        }
    }
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
        PluginEventData* event_data = dynamic_cast<PluginEventData*>(context.pluginData());
        const bool valid_data = event_data != nullptr && event_data->data() != nullptr;
        const UString name(context.pluginName());
        const bool success = _callback(context.eventCode(),
                                       name.data(),
                                       name.size() * sizeof(UChar),
                                       context.pluginIndex(),
                                       context.pluginCount(),
                                       size_t(context.bitrate().toInt()),
                                       size_t(context.pluginPackets()),
                                       size_t(context.totalPackets()),
                                       valid_data ? event_data->data() : &dummy,
                                       valid_data ? event_data->size() : 0,
                                       valid_data ? event_data->maxSize() : 0,
                                       !valid_data || event_data->readOnly(),
                                       event_data);
        if (!success && event_data != nullptr ) {
            event_data->setError(true);
        }
    }
}

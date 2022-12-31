//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsPluginEventHandlerRegistry.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PluginEventHandlerRegistry::PluginEventHandlerRegistry() :
    _mutex(),
    _calling_handlers(false),
    _handlers()
{
}

ts::PluginEventHandlerRegistry::Criteria::Criteria() :
    plugin_name(),
    plugin_index(),
    plugin_type(),
    event_code()
{
}

ts::PluginEventHandlerRegistry::Criteria::Criteria(uint32_t code) :
    plugin_name(),
    plugin_index(),
    plugin_type(),
    event_code(code)
{
}

ts::PluginEventHandlerRegistry::Criteria::Criteria(PluginType type) :
    plugin_name(),
    plugin_index(),
    plugin_type(type),
    event_code()
{
}

ts::PluginEventHandlerRegistry::Criteria::Criteria(const UString& name) :
    plugin_name(name),
    plugin_index(),
    plugin_type(),
    event_code()
{
}


//----------------------------------------------------------------------------
// Register an event handler.
//----------------------------------------------------------------------------

void ts::PluginEventHandlerRegistry::registerEventHandler(PluginEventHandlerInterface* handler, const Criteria& criteria)
{
    GuardMutex lock(_mutex);

    // Don't register null handlers, don't call from an event handler.
    if (handler != nullptr && !_calling_handlers) {

        // Look through the list to find an identical handler with the same criteria.
        for (const auto& it : _handlers) {
            if (it.first == handler &&
                it.second.plugin_name.identical(criteria.plugin_name) &&
                it.second.plugin_index.identical(criteria.plugin_index) &&
                it.second.plugin_type.identical(criteria.plugin_type) &&
                it.second.event_code.identical(criteria.event_code))
            {
                // Already registered, do not duplicate.
                return;
            }
        }

        // Finally add a new entry at the end of the list.
        _handlers.push_back(std::make_pair(handler, criteria));
    }
}


//----------------------------------------------------------------------------
// Unregister all occurences of an event handler.
//----------------------------------------------------------------------------

void ts::PluginEventHandlerRegistry::unregisterEventHandler(PluginEventHandlerInterface* handler)
{
    GuardMutex lock(_mutex);

    // Don't call from an event handler.
    if (!_calling_handlers) {
        if (handler == nullptr) {
            // Remove all handlers.
            _handlers.clear();
        }
        else {
            // Remove all entries with the specified handler.
            for (auto it = _handlers.begin(); it != _handlers.end(); ) {
                if (it->first == handler) {
                    it = _handlers.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoke all event handlers for a given event.
//----------------------------------------------------------------------------

void ts::PluginEventHandlerRegistry::callEventHandlers(const PluginEventContext& context) const
{
    // Keep the global lock all along the list lookup and handler executions...
    GuardMutex lock(_mutex);

    // Don't recurse.
    if (context.plugin() != nullptr && !_calling_handlers) {

        const PluginType type = context.plugin()->type();

        // Recursion protection.
        _calling_handlers = true;

        // Loop on all registered handlers.
        for (const auto& it : _handlers) {
            // For each handler, if a criteria is specified and does not match, skip this handler.
            if (it.second.event_code.set() && it.second.event_code.value() != context.eventCode()) {
                continue;
            }
            if (it.second.plugin_type.set() && it.second.plugin_type.value() != type) {
                continue;
            }
            if (it.second.plugin_index.set() && it.second.plugin_index.value() != context.pluginIndex()) {
                continue;
            }
            if (it.second.plugin_name.set() && it.second.plugin_name.value() != context.pluginName()) {
                continue;
            }
            // No negative criteria, call the handler.
            try {
                it.first->handlePluginEvent(context);
            }
            catch (...) {
                // Absorb handler exceptions without notification.
            }
        }

        // End of recursion protection.
        _calling_handlers = false;
    }
}

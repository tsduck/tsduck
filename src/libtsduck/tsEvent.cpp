//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Implement an "event". See also tsEventHandler.h.
//
//----------------------------------------------------------------------------

#include "tsEvent.h"
#include "tsEventHandler.h"



//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Event::Event () :
    _state (DIRECT),
    _handlers (),
    _deferred_add (),
    _deferred_remove ()
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Event::~Event ()
{
    _state = DESTROYING;

    // Cleanup all subscriptions

    for (HandlerMap::iterator i= _handlers.begin(); i != _handlers.end(); ++i) {
        i->first->removeReference (this);
    }
    _handlers.clear ();
}


//----------------------------------------------------------------------------
// Subscribe an event handler.
//----------------------------------------------------------------------------

void ts::Event::subscribe (EventHandler* handler, void* handler_arg)
{
    switch (_state) {
        case DIRECT: {
            // Direct insertion of subscription
            HandlerMap::iterator it = _handlers.find (handler);
            if (it != _handlers.end ()) {
                // Handler already registered, simply modify arg
                it->second = handler_arg;
            }
            else {
                // Handler not registered, add it
                _handlers [handler] = handler_arg;
                handler->addReference (this);
            }
            break;
        }
        case DEFERRED:
            // Will add later, after end of current notification
            _deferred_add [handler] = handler_arg;
            break;
        case DESTROYING:
            // Should not happen...
            assert (false);
            break;
    }
}


//----------------------------------------------------------------------------
// Unsubscribe an event handler.
//----------------------------------------------------------------------------

void ts::Event::unsubscribe (EventHandler* handler)
{
    switch (_state) {
        case DIRECT: {
            // Direct removal of subscription
            HandlerMap::iterator it = _handlers.find (handler);
            if (it != _handlers.end ()) {
                handler->removeReference (this);
                _handlers.erase (it);
            }
            break;
        }
        case DEFERRED:
            // Will remove later, after end of current notification
            _deferred_remove.insert (handler);
            break;
        case DESTROYING:
            // Callback from a handler we are currently removing, ignore
            break;
    }
}


//----------------------------------------------------------------------------
// Notify the event. All subscribed handlers are notified.
//----------------------------------------------------------------------------

void ts::Event::notify()
{
    // Ignore nested notification.

    if (_state != DIRECT) {
        return;
    }

    // Notify all handlers

    _state = DEFERRED;
    for (HandlerMap::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        it->first->eventNotified (this, it->second);
    }
    _state = DIRECT;

    // Process deferred add and remove (from EventNotified callbacks)

    for (HandlerMap::iterator it = _deferred_add.begin(); it != _deferred_add.end(); ++it) {
        this->subscribe (it->first, it->second);
    }
    for (HandlerSet::iterator it = _deferred_remove.begin(); it != _deferred_remove.end(); ++it) {
        this->unsubscribe (*it);
    }
    _deferred_add.clear();
    _deferred_remove.clear();
}

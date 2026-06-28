//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveBase.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveBase::ReactiveBase(Reactor& reactor, Object* owner) :
    ReporterBase(&reactor, owner),
    _reactor(reactor)
{
}

ts::ReactiveBase::~ReactiveBase()
{
    // Delete all registered ids in the reactor.
    deactivateQueuedOperations(true);
}


//----------------------------------------------------------------------------
// Signal the completed event, so that processQueuedOperations() is called.
//----------------------------------------------------------------------------

// The completion event is created the first time it is used.
bool ts::ReactiveBase::signalQueuedOperations()
{
    if (!_queued_ops_event_id.isValid()) {
        _queued_ops_event_id = _reactor.newEvent(this);
        if (!_queued_ops_event_id.isValid()) {
            return false;
        }
    }
    return _reactor.signalEvent(_queued_ops_event_id);
}

// Deactivate and delete the user event for processQueuedOperations().
void ts::ReactiveBase::deactivateQueuedOperations(bool silent)
{
    if (_queued_ops_event_id.isValid()) {
        _reactor.deleteEvent(_queued_ops_event_id, silent);
        _queued_ops_event_id.invalidate();
    }
}

// Called when an internal user-defined event is specified.
void ts::ReactiveBase::handleUserEvent(Reactor& reactor, EventId id)
{
    if (id == _queued_ops_event_id) {
        processQueuedOperations();
    }
}

// The default implementation does nothing.
void ts::ReactiveBase::processQueuedOperations()
{
}

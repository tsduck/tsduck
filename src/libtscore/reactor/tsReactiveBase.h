//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for reactive classes in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactor.h"

namespace ts {
    //!
    //! Virtual base class for reactive classes in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveBase: public ReporterBase, protected ReactorHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveBase);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveBase(Reactor& reactor, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveBase() override;

        //!
        //! Get a reference to the associated reactor.
        //! @return A reference to the associated reactor.
        //!
        Reactor& reactor() { return _reactor; }

        //!
        //! Trigger the execution of processQueuedOperations() in the context of a Reactor handler.
        //! Create if necessary and then signal a dedicated user event.
        //! @return True on success, false on error.
        //!
        bool signalQueuedOperations();

    protected:
        //!
        //! Deactivate the execution of processQueuedOperations() in the context of a Reactor handler.
        //! Deactivate and delete the dedicated user event.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateQueuedOperations(bool silent);

        //!
        //! Create, if necessary, the dedicated user event for signalQueuedOperations().
        //! Useless if signalQueuedOperations() is used. Only required with use of uncheckedSignalQueuedOperations().
        //! @return True on success, false on error.
        //!
        bool createSignalQueuedOperations();

        //!
        //! Trigger the execution of processQueuedOperations() from another thread.
        //! The event must have been previously created, either using createSignalQueuedOperations() or signalQueuedOperations().
        //! @return True on success, false on error.
        //!
        bool uncheckedSignalQueuedOperations();

        //!
        //! This virtual method processes operations in the context of a Reactor handler.
        //! This is dedicated to operations which must be serialized from an application perspective.
        //! These operations are typically queued when triggered from a method which is called by the application.
        //! When the reactor processes events, we are sure that the application is not executing a handler.
        //! The default implementation does nothing. A subclass should override it if it calls signalQueuedOperations().
        //!
        virtual void processQueuedOperations();

        // Implementation of Reactor handlers.
        // If overriden by a subclass, the subclass must call the superclass handler.
        virtual void handleUserEvent(Reactor&, EventId) override;

    private:
        Reactor& _reactor;
        EventId  _queued_ops_event_id {};
    };
}

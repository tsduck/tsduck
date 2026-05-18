//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for sockets in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOwnedObject.h"
#include "tsReactor.h"
#include "tsSocket.h"
#include "tsSysUtils.h"
#include "tsFatal.h"

namespace ts {
    //!
    //! Virtual base class for sockets in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveBase: public OwnedObject, protected ReactorHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveBase);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveBase(Reactor& reactor, Socket& socket, Object* owner = nullptr);

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
        //! Get a reference to the associated report.
        //! @return A reference to the associated report.
        //!
        Report& report() { return _reactor.report(); }

    protected:
        //! IOSB shortcut fpr subclasses.
        using IOSB = NonBlockingDevice::IOSB;

        //!
        //! Queues of I/O requests are queues of shared_ptr to IOSB.
        //! This is typically used with non-blocking I/O where we must process requests in order.
        //! Send and receive requests are structures which are stored in the react_data of the IOSB.
        //!
        using IOQueue = std::list<std::shared_ptr<IOSB>>;

        //!
        //! Unordered set of I/O requests, set of shared_ptr to IOSB.
        //! This is typically used with asynchronous I/O. The ordering is enforced because I/O are
        //! started in order of calls from applications. The completion processing is likely the
        //! same, but driven by the system I/O Completion Ports and we must not assume any order.
        //! Send and receive requests are structures which are stored in the react_data of the IOSB.
        //!
        using IOSet = std::set<std::shared_ptr<IOSB>>;

        //!
        //! Search and remove a shared_ptr to IOSB, based on an IOSB address.
        //! Search from the front (end) of the queue since a completed I/O is likely on the front.
        //! @param [in,out] queue The queue from which to remove @a iosb.
        //! @param [in] iosb Standard pointer to an IOSB to search and remove.
        //! @return The removed shared_ptr to IOSB, or a null pointer if @a iosb is not found.
        //!
        std::shared_ptr<IOSB> removeFromQueue(IOQueue& queue, IOSB* iosb);

        //!
        //! Transfer all requests from one queue to another and mark all I/O as canceled.
        //! @tparam REQUEST The subclass of Object which is set in react_data of all requests in @a inqueue.
        //! @param [in,out] inqueue The queue from which all requests are removed.
        //! @param [in,out] outqueue The queue which receives all canceled requests.
        //!
        template <class REQUEST> requires std::derived_from<REQUEST, ts::Object>
        void cancelQueue(IOQueue& inqueue, IOQueue& outqueue);

        //!
        //! Trigger the execution of processCompletedIO() in the context of a Reactor handler.
        //! Create if necessary and then signal a dedicated user event.
        //! @return True on success, false on error.
        //!
        bool signalCompletedIO();

        //!
        //! Deactivate the execution of processCompletedIO() in the context of a Reactor handler.
        //! Deactivate and delete the dedicated user event.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateCompletedIO(bool silent);

        //!
        //! This virtual method process completed I/O operations in the context of a Reactor handler.
        //! The default implementation does nothing. A subclass should override it if it calls signalCompletedIO().
        //!
        virtual void processCompletedIO();

        //!
        //! Activate read-ready notification for non-blocking I/O.
        //! @return True on success, false on error.
        //!
        bool activateReadReady();

        //!
        //! Deactivate read-ready notification for non-blocking I/O.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateReadReady(bool silent);

        //!
        //! Activate write-ready notification for non-blocking I/O.
        //! @return True on success, false on error.
        //!
        bool activateWriteReady();

        //!
        //! Deactivate write-ready notification for non-blocking I/O.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateWriteReady(bool silent);

        //!
        //! Activate notification for asynchronous I/O.
        //! @return True on success, false on error.
        //!
        bool activateAsynchronousIO();

        //!
        //! Deactivate notification for asynchronous I/O.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateAsynchronousIO(bool silent);

        //!
        //! Cancel all asynchronous I/O in progress.
        //! The cancelation occurs in the background and end of canceled asynchronous I/O will be notified.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void cancelAsynchronousIO(bool silent);

        //!
        //! Cancel one specific pending asynchronous I/O and wait for its completion.
        //! Warning: This is a blocking call. It shall be used in case of trouble only.
        //! @param [in,out] iosb The asynchronous I/O status block.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool cancelAndWaitAsynchronousIO(NonBlockingDevice::IOSB& iosb, bool silent);

        //!
        //! Deactivate all registrations for non-blocking and asynchronous I/O.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void deactivateAll(bool silent);

        // Implementation of Reactor handlers.
        // If overriden by a subclass, the subclass must call the superclass handler.
        virtual void handleUserEvent(Reactor&, EventId) override;

    private:
        Reactor& _reactor;
        Socket&  _generic_socket;
        EventId  _completed_io_id {};  // Reactor id for the completed I/O method.
        EventId  _write_ready_id {};   // Reactor id for write-ready (non-blocking I/O).
        EventId  _read_ready_id {};    // Reactor id for read-ready (non-blocking I/O).
        EventId  _async_io_id {};      // Reactor id for I/O completion (asynchronous I/O).
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Transfer all requests from one queue to another and mark all I/O as canceled.
template <class REQUEST> requires std::derived_from<REQUEST, ts::Object>
void ts::ReactiveBase::cancelQueue(IOQueue& inqueue, IOQueue& outqueue)
{
    while (!inqueue.empty()) {
        // Remove front element from input queue.
        auto iosb = inqueue.front();
        inqueue.pop_front();
        // Set canceled status in the request.
        auto req = std::dynamic_pointer_cast<REQUEST>(iosb->react_data);
        assert(req != nullptr);
        iosb->error_code = SYS_CANCELED;
        // Transfer the canceled request in output queue.
        outqueue.push_back(iosb);
    }
}

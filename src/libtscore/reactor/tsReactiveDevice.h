//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for non-blocking devices in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveBase.h"
#include "tsNonBlockingDevice.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Base class for non-blocking devices in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveDevice: public ReactiveBase
    {
        TS_NOBUILD_NOCOPY(ReactiveDevice);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] device Associated non-blocking device. The device object must remain valid as long as this object is valid.
        //!
        ReactiveDevice(Reactor& reactor, NonBlockingDevice& device);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveDevice() override;

        //!
        //! Get a reference to the associated non-blocking device.
        //! @return A reference to the associated non-blocking device.
        //!
        NonBlockingDevice& device() { return _device; }

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

    private:
        NonBlockingDevice& _device;
        EventId _write_ready_id {};     // Reactor id for write-ready (non-blocking I/O).
        EventId _read_ready_id {};      // Reactor id for read-ready (non-blocking I/O).
        EventId _read_async_io_id {};   // Reactor id for I/O completion (asynchronous I/O), read (and write if same handle).
        EventId _write_async_io_id {};  // Reactor id for I/O completion (asynchronous I/O), write (if not the same handle).
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Transfer all requests from one queue to another and mark all I/O as canceled.
template <class REQUEST> requires std::derived_from<REQUEST, ts::Object>
void ts::ReactiveDevice::cancelQueue(IOQueue& inqueue, IOQueue& outqueue)
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

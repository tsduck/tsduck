//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for all sorts of demux from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"

namespace ts {

    class TSPacket;
    class DuckContext;

    //!
    //! Abstract base class for all sorts of demux from TS packets.
    //!
    //! The application sets a number of PID's to filter. What is extracted
    //! from those PID's and how they are reported to the application depend
    //! on the concrete demux class.
    //!
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractDemux
    {
        TS_NOBUILD_NOCOPY(AbstractDemux);
    public:
        //!
        //! The following method feeds the demux with a TS packet.
        //! @param [in] pkt A TS packet.
        //!
        virtual void feedPacket(const TSPacket& pkt);

        //!
        //! Replace the list of PID's to filter.
        //! The method resetPID() is invoked on each removed PID.
        //! @param [in] pid_filter The list of PID's to filter.
        //!
        virtual void setPIDFilter(const PIDSet& pid_filter);

        //!
        //! Add one PID to filter.
        //! @param [in] pid The new PID to filter.
        //!
        virtual void addPID(PID pid);

        //!
        //! Add several PID's to filter.
        //! @param [in] pids The list of new PID's to filter.
        //!
        virtual void addPIDs(const PIDSet& pids);

        //!
        //! Remove one PID to filter.
        //! The method resetPID() is invoked on @a pid.
        //! @param [in] pid The PID to no longer filter.
        //!
        virtual void removePID(PID pid);

        //!
        //! Get the current number of PID's being filtered.
        //! @return The current number of PID's being filtered.
        //!
        virtual size_t pidCount() const;

        //!
        //! Check if a PID is filtered.
        //! @param [in] pid The PID to test.
        //! @return Tue if @a pid is filtered.
        //!
        virtual bool hasPID(PID pid) const;

        //!
        //! Reset the demux.
        //!
        //! Useful when the transport stream changes.
        //! The PID filter and the handlers are not modified.
        //!
        //! If invoked in an application-handler, the operation is delayed until
        //! the handler terminates. For subclass implementers, see beforeCallingHandler()
        //! and override immediateReset() instead of reset().
        //!
        virtual void reset();

        //!
        //! Reset the demuxing context for one single PID.
        //! Forget all previous partially demuxed data on this PID.
        //!
        //! If invoked in an application-handler, the operation is delayed until
        //! the handler terminates. For subclass implementers, see beforeCallingHandler()
        //! and override immediateResetPID() instead of resetPID().
        //!
        //! @param [in] pid The PID to reset.
        //!
        virtual void resetPID(PID pid);

        //!
        //! Set some arbitrary "demux id" value.
        //! This value is chosen and set by the application.
        //! It can be retrieved later if a table or section handler is used by several demux.
        //! The demux id is not interpreted by the demux, it is only stored for the application.
        //! The initial value of a demux id is zero.
        //! @param [in] id Application-defined demux id to assign.
        //!
        void setDemuxId(int id) { _demux_id = id; }

        //!
        //! Get the "demux id" value, as previously stored by the application.
        //! @return The application-defined demux id.
        //!
        int demuxId() const { return _demux_id; }

        //!
        //! Destructor.
        //!
        virtual ~AbstractDemux();

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] pid_filter The initial set of PID's to demux.
        //!
        explicit AbstractDemux(DuckContext& duck, const PIDSet& pid_filter = NoPID);

        //!
        //! Helper for subclass, before invoking an application-defined handler.
        //!
        //! The idea is to protect the integrity of the demux during the execution
        //! of an application-defined handler. The handler is invoked in the middle
        //! of an operation but the handler may call reset() or resetPID(). Executing
        //! the reset in the middle of an operation may be problematic. By using
        //! beforeCallingHandler() and afterCallingHandler(), all reset operations
        //! in between are delayed after the execution of the handler.
        //!
        //! Example:
        //! @code
        //! beforeCallingHandler(pid);
        //! try {
        //!     _handler->handleEvent(*this, pid, ...);
        //! }
        //! catch (...) {
        //!     afterCallingHandler(false);
        //!     throw;
        //! }
        //! afterCallingHandler();
        //! @endcode
        //!
        //! @param [in] pid The PID for which the handler will be called. All reset
        //! operations on this PID will be delayed until afterCallingHandler().
        //!
        void beforeCallingHandler(PID pid = PID_NULL);

        //!
        //! Helper for subclass, after invoking an application-defined handler.
        //! @param [in] executeDelayedOperations When true (the default), execute all pending reset operations.
        //! @return True if a delayed reset was executed.
        //! @see beforeCallingHandler()
        //!
        bool afterCallingHandler(bool executeDelayedOperations = true);

        //!
        //! Reset the demux immediately.
        //!
        virtual void immediateReset();

        //!
        //! Reset the demuxing context for one single PID immediately.
        //! @param [in] pid The PID to reset.
        //!
        virtual void immediateResetPID(PID pid);

        // Protected directly accessible to subclasses.
        DuckContext&  _duck;             //!< The TSDuck execution context is accessible to all subclasses.
        PIDSet        _pid_filter {};    //!< Current set of filtered PID's.
        PacketCounter _packet_count = 0; //!< Number of TS packets in the demultiplexed stream.

    private:
        bool _in_handler = false;        // True when in the context of an application-defined handler
        PID  _pid_in_handler = PID_NULL; // PID which is currently processed by the handler
        bool _reset_pending = false;     // Delayed reset()
        bool _pid_reset_pending = false; // Delayed resetPID(_pid_in_handler)
        int  _demux_id = 0;              // Demux identity (from application)
    };
}

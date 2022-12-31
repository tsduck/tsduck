//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!
//!  @file
//!  Base class for Digital TV tuners.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDuckContext.h"

namespace ts {

    class TunerBase;
    class AbortInterface;
    class Report;
    class ModulationArgs;
    class SignalState;
    class TSPacket;
    class DeliverySystemSet;

    //!
    //! Safe pointer to a tuner (not thread-safe).
    //!
    typedef SafePtr<TunerBase, NullMutex> TunerPtr;

    //!
    //! Vector of safe pointers to tuners (not thread-safe).
    //!
    typedef std::vector<TunerPtr> TunerPtrVector;

    //!
    //! Base class for Digital TV tuners.
    //! @ingroup hardware
    //!
    //! The TunerBase class defines all virtual methods to access a tuner.
    //! All services in the base class are "unimplemented" and return an error.
    //! Actual services should be implemented by subclasses.
    //!
    //! The main subclasses are TunerDevice which implements a physical tuner,
    //! TunerEmulator which implements a file-based fake tuner and Tuner which
    //! encapsulates both capabilities.
    //!
    //! A note on history: In older versions of TSDuck, a tuner had a single "type"
    //! (DVT-T, DVB-S, etc.). There was also a specific subclass of tuner parameters
    //! for each type of tuner. With the advent of multi-standard tuners (DVB-T and
    //! DVB-C for instance), this was no longer appropriate. Now, each tuner device
    //! has a set of supported delivery systems. There is one single class containing
    //! all tuning parameters for all delivery systems. The selected delivery system
    //! is one of these parameter. To tune a device, we now provide an instance of
    //! the ModulationArgs class. If the tuner supports the target delivery system, it
    //! picks the appropriate parameters for the selected delivery system.
    //!
    class TSDUCKDLL TunerBase
    {
    public:
        //!
        //! Get the list of all existing physical tuners.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] tuners Returned list of physical tuners on the system.
        //! @return True on success, false on error.
        //!
        static bool GetAllTuners(DuckContext& duck, TunerPtrVector& tuners);

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        TunerBase(DuckContext& duck);

        //!
        //! Virtual destructor.
        //!
        virtual ~TunerBase();

        //!
        //! Open the tuner.
        //! @param [in] device_name Tuner device name. If name is empty, use "first" or "default" tuner.
        //! @param [in] info_only If true, we will only fetch the properties of the tuner, we won't use it to
        //! receive streams. Thus, it is possible to open tuners which are already used to actually receive a stream.
        //! @return True on success, false on error.
        //!
        virtual bool open(const UString& device_name, bool info_only);

        //!
        //! Close the tuner.
        //! @param [in] silent When true, do not report close errors.
        //! @return True on success, false on error.
        //!
        virtual bool close(bool silent = false);

        //!
        //! Check if the tuner is open.
        //! @return True if the tuner is open.
        //!
        virtual bool isOpen() const;

        //!
        //! Get the open mode.
        //! @return True if the tuner is open to fetch information only.
        //! In that case, the tuner cannot receive streams.
        //!
        virtual bool infoOnly() const;

        //!
        //! Set of delivery systems which are supported by the tuner.
        //! @return A constant reference to the set of delivery systems which are supported by the tuner.
        //!
        virtual const DeliverySystemSet& deliverySystems() const;

        //!
        //! Get the device name of the tuner.
        //! @return The device name of the tuner.
        //!
        virtual UString deviceName() const;

        //!
        //! Device-specific information.
        //! @return A string with device-specific information. Can be empty.
        //!
        virtual UString deviceInfo() const;

        //!
        //! System-specific device path (for information only).
        //! @return A string with system-specific device path. Can be empty.
        //!
        virtual UString devicePath() const;

        //!
        //! Check if a signal is present and get the signal state.
        //! @param [out] state Returned state of the tuner. Some fields may be unset if unavailable with that tuner.
        //! @return True in case of success (even if no signal was detected), false on error.
        //!
        virtual bool getSignalState(SignalState& state);

        //!
        //! Tune to the specified parameters.
        //! @param [in,out] params Tuning parameters. Updated with missing default values.
        //! @return True on success, false on error.
        //!
        virtual bool tune(ModulationArgs& params);

        //!
        //! Start receiving packets.
        //! @return True on success, false on error.
        //!
        virtual bool start();

        //!
        //! Stop receiving packets.
        //! @param [in] silent When true, do not report close errors.
        //! @return True on success, false on error.
        //!
        virtual bool stop(bool silent = false);

        //!
        //! Abort any pending or blocked reception.
        //! This unblocks a blocked reader but leaves the tuner in an undefined state.
        //! The only safe option after this is a close().
        //! @param [in] silent When true, do not report close errors.
        //!
        virtual void abort(bool silent = false);

        //!
        //! Receive packets.
        //! @param [out] buffer Address of TS packet receive buffer.
        //! Read only complete 188-byte TS packets.
        //! @param [in] max_packets Maximum number of packets to read in @a buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return The number of actually received packets (in the range 1 to @a max_packets).
        //! Returning zero means error or end of input.
        //!
        virtual size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort = nullptr);

        //!
        //! Get the current tuning parameters.
        //! @param [in,out] params Returned tuning parameters.
        //! Modify only the properties which can be reported by the tuner.
        //! @param [in] reset_unknown If true, the unknown values (those
        //! which are not reported by the tuner) are reset to unknown/zero/auto
        //! values. Otherwise, they are left unmodified.
        //! @return True on success, false on error.
        //!
        virtual bool getCurrentTuning(ModulationArgs& params, bool reset_unknown);

        //!
        //! Default timeout before getting a signal on start.
        //!
        static constexpr MilliSecond DEFAULT_SIGNAL_TIMEOUT = 5000; // 5 seconds

        //!
        //! Set the timeout before getting a signal on start.
        //! If zero, do not wait for signal on start.
        //! Must be set before start().
        //! @param [in] t Number of milliseconds to wait after start() before receiving a signal.
        //!
        virtual void setSignalTimeout(MilliSecond t);

        //!
        //! Set if an error should be reported on timeout before getting a signal on start.
        //! Must be set before start().
        //! @param [in] silent If true, no error message will be reported if no signal is received after the timeout on start.
        //!
        virtual void setSignalTimeoutSilent(bool silent);

        //!
        //! Set the timeout for receive operations.
        //! @param [in] t Number of milliseconds to wait before receiving
        //! packets in a receive() operation. If zero (the default), no
        //! timeout is applied.
        //! @return True on success, false on error.
        //!
        virtual bool setReceiveTimeout(MilliSecond t);

        //!
        //! Get the timeout for receive operation.
        //! @return The timeout for receive operation.
        //! @see setReceiveTimeout()
        //!
        virtual MilliSecond receiveTimeout() const;

        //!
        //! Default poll interval for signal timeout (Linux-specific).
        //!
        static constexpr MilliSecond DEFAULT_SIGNAL_POLL = 100;

        //!
        //! Set the poll interval for signal timeout (Linux-specific).
        //! Must be set before start().
        //! This is a Linux-specific method which does nothing on other systems.
        //! @param [in] t Poll interval in milliseconds.
        //!
        virtual void setSignalPoll(MilliSecond t);

        //!
        //! Default demux buffer size in bytes (Linux-specific).
        //!
        static constexpr size_t DEFAULT_DEMUX_BUFFER_SIZE = 1024 * 1024;  // 1 MB

        //!
        //! Set the demux buffer size in bytes (Linux-specific).
        //! Must be set before start().
        //! This is a Linux-specific method which does nothing on other systems.
        //! @param [in] s The demux buffer size in bytes.
        //!
        virtual void setDemuxBufferSize(size_t s);

        //!
        //! Default max number of queued media samples (Windows-specific).
        //! @see setSinkQueueSize().
        //!
        static constexpr size_t DEFAULT_SINK_QUEUE_SIZE = 1000;  // media samples

        //!
        //! Set the max number of queued media samples (Windows-specific).
        //! Must be set before start().
        //! This is a Windows-specific method which does nothing on other systems.
        //! @param [in] s Max number of media samples in the queue between
        //! the graph thread and the application thread.
        //!
        virtual void setSinkQueueSize(size_t s);

        //!
        //! Specify a receiver filter name (Windows-specific).
        //! Must be set before open().
        //! This is a Windows-specific method which does nothing on other systems.
        //! @param [in] name Name of the receiver filter to use. The DirectShow graph will
        //! use the specified receiver filter instead of the standard search algorithm.
        //!
        virtual void setReceiverFilterName(const UString& name);

        //!
        //! Display the characteristics and status of the tuner.
        //! @param [in,out] strm Output text stream.
        //! @param [in] margin Left margin to display.
        //! @param [in] extended Display "extended" information. Can be very verbose.
        //! @return A reference to @a strm.
        //!
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin = UString(), bool extended = false);

        //!
        //! Get a reference to the error report.
        //! @return A reference to the error report.
        //!
        Report& report() const { return _duck.report(); }

    protected:
        DuckContext& _duck; //!< TSDuck execution context for subclasses.

        //!
        //! Check the consistency of tune() parameters.
        //! @param [in,out] params Modulation parameters. Updated with default values.
        //! @return True on success, false on error.
        //!
        bool checkTuneParameters(ModulationArgs& params) const;

        //!
        //! Helper for unimplemented methods.
        //! Display a standard error message.
        //! @return False.
        //!
        bool unimplemented() const;
    };
}

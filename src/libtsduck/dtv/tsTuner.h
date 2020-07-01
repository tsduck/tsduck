//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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
//!
//!  @file
//!  Digital TV tuner.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsModulationArgs.h"
#include "tsAbortInterface.h"
#include "tsSafePtr.h"
#include "tsReport.h"

namespace ts {

    class Tuner;

    //!
    //! Safe pointer to a tuner (not thread-safe).
    //!
    typedef SafePtr<Tuner, NullMutex> TunerPtr;

    //!
    //! Vector of safe pointers to tuners (not thread-safe).
    //!
    typedef std::vector<TunerPtr> TunerPtrVector;

    //!
    //! Implementation of a digital TV tuner.
    //! @ingroup hardware
    //!
    //! The syntax of a tuner "device name" depends on the operating system.
    //!
    //! Linux:
    //! - Syntax: /dev/dvb/adapterA[:F[:M[:V]]]
    //! - A = adapter number
    //! - F = frontend number (default: 0)
    //! - M = demux number (default: 0)
    //! - V = dvr number (default: 0)
    //!
    //! Windows:
    //! - DirectShow/BDA tuner filter name
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
    class TSDUCKDLL Tuner
    {
        TS_NOBUILD_NOCOPY(Tuner);
    public:
        //!
        //! Get the list of all existing DVB tuners.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] tuners Returned list of DVB tuners on the system.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAllTuners(DuckContext& duck, TunerPtrVector& tuners, Report& report);

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        Tuner(DuckContext& duck);

        //!
        //! Destructor.
        //!
        ~Tuner();

        //!
        //! Constructor and open device name.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] device_name Tuner device name.
        //! If name is empty, use "first" or "default" tuner.
        //! @param [in] info_only If true, we will only fetch the properties of
        //! the tuner, we won't use it to receive streams. Thus, it is possible
        //! to open tuners which are already used to actually receive a stream.
        //! @param [in,out] report Where to report errors.
        //!
        Tuner(DuckContext& duck, const UString& device_name, bool info_only, Report& report);

        //!
        //! Open the tuner.
        //! @param [in] device_name Tuner device name.
        //! If name is empty, use "first" or "default" tuner.
        //! @param [in] info_only If true, we will only fetch the properties of
        //! the tuner, we won't use it to receive streams. Thus, it is possible
        //! to open tuners which are already used to actually receive a stream.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(const UString& device_name, bool info_only, Report& report);

        //!
        //! Close the tuner.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Check if the tuner is open.
        //! @return True if the tuner is open.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Get the open mode.
        //! @return True if the tuner is open to fetch information only.
        //! In that case, the tuner cannot receive streams.
        //!
        bool infoOnly() const { return _info_only; }

        //!
        //! Set of delivery systems which are supported by the tuner.
        //! @return A constant reference to the set of delivery systems which are supported by the tuner.
        //!
        const DeliverySystemSet& deliverySystems() const { return _delivery_systems; }

        //!
        //! Get the device name of the tuner.
        //! @return The device name of the tuner.
        //!
        UString deviceName() const { return _device_name; }

        //!
        //! Device-specific information.
        //! @return A string with device-specific information. Can be empty.
        //!
        UString deviceInfo() const { return _device_info; }

        //!
        //! System-specific device path (for information only).
        //! @return A string with system-specific device path. Can be empty.
        //!
        UString devicePath() const { return _device_path; }

        //!
        //! Check if a signal is present and locked.
        //! @param [in,out] report Where to report errors.
        //! @return True if a signal is present and locked.
        //!
        bool signalLocked(Report& report);

        //!
        //! Get the signal strength.
        //! @param [in,out] report Where to report errors.
        //! @return The signal strength in percent (0=bad, 100=good).
        //! Return a negative value on error.
        //!
        int signalStrength(Report& report);

        //!
        //! Get the signal quality.
        //! @param [in,out] report Where to report errors.
        //! @return The signal quality in percent (0=bad, 100=good).
        //! Return a negative value on error.
        //!
        int signalQuality(Report& report);

        //!
        //! Tune to the specified parameters.
        //! @param [in,out] params Tuning parameters. Updated with missing default values.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool tune(ModulationArgs& params, Report& report);

        //!
        //! Start receiving packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool start(Report& report);

        //!
        //! Stop receiving packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool stop(Report& report);

        //!
        //! Receive packets.
        //! @param [out] buffer Address of TS packet receive buffer.
        //! Read only complete 188-byte TS packets.
        //! @param [in] max_packets Maximum number of packets to read in @a buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report errors.
        //! @return The number of actually received packets (in the range 1 to @a max_packets).
        //! Returning zero means error or end of input.
        //!
        size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report);

        //!
        //! Get the current tuning parameters.
        //! @param [in,out] params Returned tuning parameters.
        //! Modify only the properties which can be reported by the tuner.
        //! @param [in] reset_unknown If true, the unknown values (those
        //! which are not reported by the tuner) are reset to unknown/zero/auto
        //! values. Otherwise, they are left unmodified.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report);

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
        void setSignalTimeout(MilliSecond t);

        //!
        //! Set if an error should be reported on timeout before getting a signal on start.
        //! Must be set before start().
        //! @param [in] silent If true, no error message will be reported if no signal is
        //! received after the timeout on start.
        //!
        void setSignalTimeoutSilent(bool silent);

        //!
        //! Set the timeout for receive operations.
        //! @param [in] t Number of milliseconds to wait before receiving
        //! packets in a receive() operation. If zero (the default), no
        //! timeout is applied.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimeout(MilliSecond t, Report& report);

        //!
        //! Get the timeout for receive operation.
        //! @return The timeout for receive operation.
        //! @see setReceiveTimeout()
        //!
        MilliSecond receiveTimeout() const { return _receive_timeout; }

#if defined(TS_LINUX) || defined(DOXYGEN) // Linux-specific operations
        //!
        //! Default poll interval for signal timeout (Linux-specific).
        //!
        static constexpr MilliSecond DEFAULT_SIGNAL_POLL = 100;

        //!
        //! Set the poll interval for signal timeout (Linux-specific).
        //! Must be set before start().
        //! @param [in] t Poll interval in milliseconds.
        //!
        void setSignalPoll(MilliSecond t);

        //!
        //! Default demux buffer size in bytes (Linux-specific).
        //!
        static constexpr size_t DEFAULT_DEMUX_BUFFER_SIZE = 1024 * 1024;  // 1 MB

        //!
        //! Set the demux buffer size in bytes.
        //! Must be set before start().
        //! @param [in] s The demux buffer size in bytes.
        //!
        void setDemuxBufferSize(size_t s);
#endif

#if defined(TS_WINDOWS) || defined(DOXYGEN) // Windows-specific operations
        //!
        //! Default max number of queued media samples (Windows-specific).
        //! @see setSinkQueueSize().
        //!
        static constexpr size_t DEFAULT_SINK_QUEUE_SIZE = 1000;  // media samples

        //!
        //! Set the max number of queued media samples (Windows-specific).
        //! Must be set before start().
        //! @param [in] s Max number of media samples in the queue between
        //! the graph thread and the application thread.
        //!
        void setSinkQueueSize(size_t s);

        //!
        //! Specify a receiver filter name.
        //! Must be set before open().
        //! @param [in] name Name of the receiver filter to use. The DirectShow graph will
        //! use the specified receiver filter instead of the standard search algorithm.
        //!
        void setReceiverFilterName(const UString& name);
#endif

        //!
        //! Display the characteristics and status of the tuner.
        //! @param [in,out] strm Output text stream.
        //! @param [in] margin Left margin to display.
        //! @param [in,out] report Where to report errors.
        //! @return A reference to @a strm.
        //!
        std::ostream& displayStatus(std::ostream& strm, const UString& margin, Report& report);

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class Guts;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Check the consistency of tune() parameters.
        // Return full parameters with default values.
        // Return true on success, false on error.
        bool checkTuneParameters(ModulationArgs& params, Report& report) const;

        // Private members.
        DuckContext&      _duck;
        bool              _is_open;
        bool              _info_only;
        UString           _device_name;    // Used to open the tuner
        UString           _device_info;    // Device-specific, can be empty
        UString           _device_path;    // System-specific device path.
        MilliSecond       _signal_timeout;
        bool              _signal_timeout_silent;
        MilliSecond       _receive_timeout;
        DeliverySystemSet _delivery_systems;
        Guts*             _guts;           // System-specific data
    };
}

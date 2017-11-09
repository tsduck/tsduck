//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
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
//!  DVB tuner.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsModulation.h"
#include "tsAbortInterface.h"
#include "tsReportInterface.h"
#include "tsTunerParameters.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersATSC.h"

#if defined(TS_LINUX) || defined(DOXYGEN)
#include "tsDTVProperties.h"
#endif

#if defined(TS_WINDOWS) || defined(DOXYGEN)
#include "tsDirectShowGraph.h"
#include "tsSinkFilter.h"
#include "tsComPtr.h"
#endif

namespace ts {

    class Tuner;

    //!
    //! Safe pointer to a DVB tuner (not thread-safe).
    //!
    typedef SafePtr<Tuner, NullMutex> TunerPtr;

    //!
    //! Vector of safe pointers to DVB tuners (not thread-safe).
    //!
    typedef std::vector<TunerPtr> TunerPtrVector;

    //!
    //! Implementation of a DVB tuner.
    //!
    //! The syntax of a DVB tuner "device name" depends on the operating system.
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
    class TSDUCKDLL Tuner
    {
    public:
        //!
        //! Get the list of all existing DVB tuners.
        //! @param [out] tuners Returned list of DVB tuners on the system.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAllTuners(TunerPtrVector& tuners, ReportInterface& report);

        //!
        //! Default constructor.
        //! @param [in] device_name Tuner device name.
        //!
        Tuner(const std::string& device_name = std::string());

        //!
        //! Destructor.
        //!
        ~Tuner();

        //!
        //! Constructor and open device name.
        //! @param [in] device_name Tuner device name.
        //! If name is empty, use "first" or "default" tuner.
        //! @param [in] info_only If true, we will only fetch the properties of
        //! the tuner, we won't use it to receive streams. Thus, it is possible
        //! to open tuners which are already used to actually receive a stream.
        //! @param [in,out] report Where to report errors.
        //!
        Tuner(const std::string& device_name, bool info_only, ReportInterface& report);

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
        bool open(const std::string& device_name, bool info_only, ReportInterface& report);

        //!
        //! Close the tuner.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(ReportInterface& report);

        //!
        //! Check if the tuner is open.
        //! @return True if the tuner is open.
        //!
        bool isOpen() const
        {
            return _is_open;
        }

        //!
        //! Get the open mode.
        //! @return True if the tuner is open to fetch information only.
        //! In that case, the tuner cannot receive streams.
        //!
        bool infoOnly() const
        {
            return _info_only;
        }

        //!
        //! Get the tuner type.
        //! @return The tuner type.
        //!
        TunerType tunerType() const
        {
            return _tuner_type;
        }

        //!
        //! Set of delivery systems which are supported by the tuner.
        //! @return The set of delivery systems which are supported by the tuner.
        //!
        DeliverySystemSet deliverySystems() const
        {
            return _delivery_systems;
        }

        //!
        //! Check if the tuner supports the specified delivery system.
        //! @param [in] ds The delivery system to check.
        //! @return True if the tuner supports the specified delivery system.
        //!
        bool hasDeliverySystem(DeliverySystem ds) const
        {
            return _delivery_systems.test(size_t(ds));
        }

        //!
        //! Get the device name of the tuner.
        //! @return The device name of the tuner.
        //!
        std::string deviceName() const
        {
            return _device_name;
        }

        //!
        //! Device-specific information.
        //! @return A string with device-specific information. Can be empty.
        //!
        std::string deviceInfo() const
        {
            return _device_info;
        }

        //!
        //! Check if a signal is present and locked.
        //! @param [in,out] report Where to report errors.
        //! @return True if a signal is present and locked.
        //!
        bool signalLocked(ReportInterface& report);

        //!
        //! Get the signal strength.
        //! @param [in,out] report Where to report errors.
        //! @return The signal strength in percent (0=bad, 100=good).
        //! Return a negative value on error.
        //!
        int signalStrength(ReportInterface& report);

        //!
        //! Get the signal quality.
        //! @param [in,out] report Where to report errors.
        //! @return The signal quality in percent (0=bad, 100=good).
        //! Return a negative value on error.
        //!
        int signalQuality(ReportInterface& report);

        //!
        //! Tune to the specified parameters.
        //! @param [in] params Tuning parameters.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool tune(const TunerParameters& params, ReportInterface& report);

        //!
        //! Start receiving packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool start(ReportInterface& report);

        //!
        //! Stop receiving packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool stop(ReportInterface& report);

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
        size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, ReportInterface& report);

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
        bool getCurrentTuning(TunerParameters& params, bool reset_unknown, ReportInterface& report);

        //!
        //! Default timeout before getting a signal on start.
        //!
        static const MilliSecond DEFAULT_SIGNAL_TIMEOUT = 5000; // 5 seconds

        //!
        //! Set the timeout before getting a signal on start.
        //! If zero, do not wait for signal on start.
        //! Must be set before start().
        //! @param [in] t Number of milliseconds to wait after start() before receiving a signal.
        //!
        void setSignalTimeout(MilliSecond t)
        {
            _signal_timeout = t;
        }

        //!
        //! Set if an error should be reported on timeout before getting a signal on start.
        //! Must be set before start().
        //! @param [in] silent If true, no error message will be reported if no signal is
        //! received after the timeout on start.
        //!
        void setSignalTimeoutSilent(bool silent)
        {
            _signal_timeout_silent = silent;
        }

        //!
        //! Set the timeout for receive operations.
        //! @param [in] t Number of milliseconds to wait before receiving
        //! packets in a receive() operation. If zero (the default), no
        //! timeout is applied.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimeout(MilliSecond t, ReportInterface& report);

        //!
        //! Get the timeout for receive operation.
        //! @return The timeout for receive operation.
        //! @see setReceiveTimeout()
        //!
        MilliSecond receiveTimeout() const
        {
            return _receive_timeout;
        }

#if defined(TS_LINUX) || defined(DOXYGEN) // Linux-specific operations

        //!
        //! Default poll interval for signal timeout (Linux-specific).
        //!
        static const MilliSecond DEFAULT_SIGNAL_POLL = 100;

        //!
        //! Set the poll interval for signal timeout (Linux-specific).
        //! Must be set before start().
        //! @param [in] t Poll interval in milliseconds.
        //!
        void setSignalPoll(MilliSecond t)
        {
            _signal_poll = t;
        }

        //!
        //! Default demux buffer size in bytes (Linux-specific).
        //!
        static const size_t DEFAULT_DEMUX_BUFFER_SIZE = 1024 * 1024;  // 1 MB

        //!
        //! Set the demux buffer size in bytes.
        //! Must be set before start().
        //! @param [in] s The demux buffer size in bytes.
        //!
        void setDemuxBufferSize(size_t s)
        {
            _demux_bufsize = s;
        }

        //!
        //! Perform a tune operation (Linux-specific).
        //! @param [in,out] props DTV properties.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool tune(DTVProperties& props, ReportInterface& report);
#endif

#if defined(TS_WINDOWS) || defined(DOXYGEN) // Windows-specific operations

        //!
        //! Default max number of queued media samples (Windows-specific).
        //! @see setSinkQueueSize().
        //!
        static const size_t DEFAULT_SINK_QUEUE_SIZE = 100;  // media samples

        //!
        //! Set the max number of queued media samples (Windows-specific).
        //! Must be set before start().
        //! @param [in] s Max number of media samples in the queue between
        //! the graph thread and the application thread.
        //!
        void setSinkQueueSize(size_t s)
        {
            _sink_queue_size = s;
        }
#endif

        //!
        //! Display the characteristics and status of the tuner.
        //! @param [in,out] strm Output text stream.
        //! @param [in] margin Left margin to display.
        //! @param [in,out] report Where to report errors.
        //! @return A reference to @a strm.
        //!
        std::ostream& displayStatus(std::ostream& strm, const std::string& margin, ReportInterface& report);

    private:

        // Portable properties
        bool              _is_open;
        bool              _info_only;
        TunerType         _tuner_type;
        std::string       _device_name;    // Used to open the tuner
        std::string       _device_info;    // Device-specific, can be empty
        MilliSecond       _signal_timeout;
        bool              _signal_timeout_silent;
        MilliSecond       _receive_timeout;
        DeliverySystemSet _delivery_systems;

#if defined(TS_LINUX) // Linux properties

        std::string         _frontend_name;    // Frontend device name
        std::string         _demux_name;       // Demux device name
        std::string         _dvr_name;         // DVR device name
        int                 _frontend_fd;      // Frontend device file descriptor
        int                 _demux_fd;         // Demux device file descriptor
        int                 _dvr_fd;           // DVR device file descriptor
        unsigned long       _demux_bufsize;    // Demux device buffer size
        ::dvb_frontend_info _fe_info;          // Front-end characteristics
        MilliSecond         _signal_poll;
        int                 _rt_signal;        // Receive timeout signal number
        ::timer_t           _rt_timer;         // Receive timeout timer
        bool                _rt_timer_valid;   // Receive timeout timer was created

        // Get current tuning parameters for specific tuners, return system error code
        ErrorCode getCurrentTuningDVBS(TunerParametersDVBS&);
        ErrorCode getCurrentTuningDVBC(TunerParametersDVBC&);
        ErrorCode getCurrentTuningDVBT(TunerParametersDVBT&);
        ErrorCode getCurrentTuningATSC(TunerParametersATSC&);

        // Clear tuner, return true on success, false on error
        bool dtvClear(ReportInterface&);

        // Discard all pending frontend events
        void discardFrontendEvents(ReportInterface&);

        // Tune for specific tuners, return true on success, false on error
        bool tuneDVBS(const TunerParametersDVBS&, ReportInterface&);
        bool tuneDVBC(const TunerParametersDVBC&, ReportInterface&);
        bool tuneDVBT(const TunerParametersDVBT&, ReportInterface&);
        bool tuneATSC(const TunerParametersATSC&, ReportInterface&);

        // Convert between TSDuck and Linux delivery systems.
        DeliverySystem fromLinuxDeliverySystem(::fe_delivery_system);
        ::fe_delivery_system toLinuxDeliverySystem(DeliverySystem);

#endif // linux

#if defined(TS_WINDOWS) // Windows properties

        // A DirectShow graph for TS capture is usually made of the following filters:
        // - Network provider (typically "Microsoft DVBx Network Provider")
        // - Tuner (typically provided by tuner hardware vendor as "BDA driver")
        // - Receiver (optional, also provided by tuner hardware vendor)
        // - Tee filter, creating two branches:
        // - Branch A: actual capture of TS packets
        //   - SinkFiler (provided by TSDuck)
        // - Branch B: MPEG-2 demux, actually unused but required by the graph
        //   - MPEG-2 demultiplexer
        //   - TIF (Transport Information Filter)

        size_t                  _sink_queue_size;     // Media sample queue size
        DirectShowGraph         _graph;               // The filter graph
        ComPtr<SinkFilter>      _sink_filter;         // Sink filter to TSDuck
        ComPtr<::IBaseFilter>   _provider_filter;     // Network provider filter
        ComPtr<::IBDA_NetworkProvider> _net_provider; // ... interface of provider_filter
        ComPtr<::ITuner>        _tuner;               // ... interface of provider_filter
        ComPtr<::ITuningSpace>  _tuning_space;        // ... associated to provider_filter
        std::string             _tuning_space_fname;  // ... friendly name
        std::string             _tuning_space_uname;  // ... unique name
        ComPtr<::IBaseFilter>   _tuner_filter;        // Tuner filter
        std::vector<ComPtr<::IBDA_DigitalDemodulator>>  _demods;   // ... all its demod interfaces
        std::vector<ComPtr<::IBDA_DigitalDemodulator2>> _demods2;  // ... all its demod (2nd gen) interfaces
        std::vector<ComPtr<::IBDA_SignalStatistics>>    _sigstats; // ... all its signal stat interfaces
        std::vector<ComPtr<::IKsPropertySet>>           _tunprops; // ... all its property set interfaces

        // Try to build the graph.
        // Return true on success, false on error
        bool buildGraph(::IMoniker* tuner_moniker, ReportInterface&);

        // Try to build the part of the graph starting at the tee filter.
        // The specified base filter is either the tuner filter or some
        // other intermediate receiver filter downstream the tuner.
        // Return true on success, false on error.
        bool buildCaptureGraph(const ComPtr <::IBaseFilter>&, ReportInterface&);

        // Internal tune method, works also if the tuner is not in open state.
        // Return true on success, false on errors
        bool internalTune(const TunerParameters&, ReportInterface&);

        // Get signal strength in mdB.
        // Return true if found, false if not found.
        bool getSignalStrength_mdB(::LONG&);

        // Locate all known interfaces in a pin or node of the tuner filter.
        // Add found interfaces in _demods, _demods2, _sigstats, _tunprops.
        // Ignore errors.
        template <class COMCLASS>
        void findTunerSubinterfaces(ComPtr<COMCLASS>&);

        // Search criteria for properties.
        enum PropSearch {psFIRST, psLAST, psLOWEST, psHIGHEST};

        // Search all IKsPropertySet in the tuner until the specified data is found.
        // Return true if found, false if not found.
        template <typename T>
        bool searchTunerProperty(const ::GUID& propset, ::DWORD propid, T& value, PropSearch);

        // Find one or more tuners. Exactly one of Tuner* or TunerPtrVector* must be non-zero.
        // If Tuner* is non-zero, find the first tuner (matching _device_name if not empty).
        // If _device_name is ":integer", use integer as device index in list of DVB devices.
        // If TunerPtrVector* is non- zero, find all tuners in the system.
        // Return true on success, false on error.
        static bool FindTuners(Tuner*, TunerPtrVector*, ReportInterface&);

#endif // windows

        // Inaccessible operations.
        Tuner(const Tuner&) = delete;
        Tuner& operator=(const Tuner&) = delete;
    };
}

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

#if defined(__linux) || defined(DOXYGEN)
#include "tsDTVProperties.h"
#endif

#if defined(__windows) || defined(DOXYGEN)
#include "tsSinkFilter.h"
#include "tsComPtr.h"
#endif

namespace ts {

    class Tuner;
    typedef SafePtr <Tuner, NullMutex> TunerPtr;
    typedef std::vector <TunerPtr> TunerPtrVector;

    class TSDUCKDLL Tuner
    {
    public:

        // Get the list of all existing DVB tuners.
        // Return true on success, false on errors
        static bool GetAllTuners(TunerPtrVector&, ReportInterface&);

        // Default constructor, 
        Tuner();

        // Destructor
        ~Tuner();

        // Constructor and open device name.
        // If name is empty, use "first" or "default" tuner.
        Tuner(const std::string& device_name, bool info_only, ReportInterface&);

        // DVB tuner "device name":
        // Linux:
        //   Syntax: /dev/dvb/adapterA[:F[:M[:V]]]
        //   A = adapter number
        //   F = frontend number (default: 0)
        //   M = demux number (default: 0)
        //   V = dvr number (default: 0)
        // Windows:
        //   DirectShow/BDA tuner filter name

        // Open the tuner.
        // If name is empty, use "first" or "default" tuner.
        // If info_only is true, cannot tune, start or receive packet.
        // Return true on success, false on errors
        bool open(const std::string& device_name, bool info_only, ReportInterface&);

        // Close tuner.
        // Return true on success, false on errors
        bool close(ReportInterface&);

        // Check if open
        bool isOpen() const {return _is_open;}

        // Get open mode
        bool infoOnly() const {return _info_only;}

        // Tuner type
        TunerType tunerType() const {return _tuner_type;}

        // Set of delivery systems which are supported by the tuner
        DeliverySystemSet deliverySystems() const {return _delivery_systems;}

        // Check if the tuner supports the specified delivery system
        bool hasDeliverySystem (DeliverySystem ds) const {return _delivery_systems.test (size_t (ds));}

        // Get the device name of a tuner.
        std::string deviceName() const {return _device_name;}

        // Device-specific, can be empty
        std::string deviceInfo() const {return _device_info;}

        // Check if a signal is present and locked
        bool signalLocked(ReportInterface&);

        // Return signal strength, in percent (0=bad, 100=good)
        // Return a negative value on error.
        int signalStrength(ReportInterface&);

        // Return signal quality, in percent (0=bad, 100=good)
        // Return a negative value on error.
        int signalQuality(ReportInterface&);

        // Tune to the specified parameters.
        // Return true on success, false on errors
        bool tune(const TunerParameters&, ReportInterface&);

        // Start receiving packets.
        // Return true on success, false on errors
        bool start(ReportInterface&);

        // Stop receiving packets.
        // Return true on success, false on errors
        bool stop(ReportInterface&);

        // Read complete 188-byte TS packets in the buffer and return the
        // number of actually received packets (in the range 1 to max_packets).
        // If abort interface is non-zero, invoke it when I/O is interrupted
        // (in case of user-interrupt, return, otherwise retry).
        // Returning zero means error or end of input.
        size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface*, ReportInterface&);

        // Get the current tuning parameters: update an existing
        // TunerParameters, modify only the properties which can
        // be reported by the tuner. If reset_unknown is true,
        // the unknown values (those which are not reported by the
        // tuner) are reset to unknown/zero/auto values.
        // Return true on success, false on errors.
        bool getCurrentTuning(TunerParameters&, bool reset_unknown, ReportInterface&);

        // Timeout before getting a signal on start.
        // If zero, do not wait for signal on start.
        // Must be set before start.
        static const MilliSecond DEFAULT_SIGNAL_TIMEOUT = 5000; // 5 seconds
        void setSignalTimeout(MilliSecond t) {_signal_timeout = t;}
        void setSignalTimeoutSilent(bool silent) {_signal_timeout_silent = silent;}

        // Timeout for receive operation (none by default).
        // If zero, no timeout is applied.
        // Return true on success, false on errors.
        bool setReceiveTimeout(MilliSecond, ReportInterface&);

#if defined(__linux) || defined(DOXYGEN) // Linux-specific operations

        // Poll interval for signal timeout. Must be set before start.
        static const MilliSecond DEFAULT_SIGNAL_POLL = 100;
        void setSignalPoll(MilliSecond t) {_signal_poll = t;}

        // Demux buffer size in bytes. Must be set before start.
        static const size_t DEFAULT_DEMUX_BUFFER_SIZE = 1024 * 1024;  // 1 MB
        void setDemuxBufferSize(size_t s) {_demux_bufsize = s;}

        // Force usage of S2API in all cases
        void setForceS2API(bool force) {_force_s2api = force;}
        bool getForceS2API() const {return _force_s2api;}

#if defined(__s2api) || defined(DOXYGEN)
        // Tune operation using S2API, return true on success, false on error
        bool tuneS2API(DTVProperties&, ReportInterface&);
#endif
#endif

#if defined(__windows) || defined(DOXYGEN) // Windows-specific operations

        // Max number of media samples in the queue between
        // the graph thread and the application thread.
        // Must be set before start.
        static const size_t DEFAULT_SINK_QUEUE_SIZE = 50;  // media samples
        void setSinkQueueSize(size_t s) {_sink_queue_size = s;}

        // Enumerate all tuner-related DirectShow devices.
        static std::ostream& EnumerateDevices(std::ostream&, const std::string& margin, ReportInterface&);
#endif

        // Display the characteristics and status of the tuner.
        std::ostream& displayStatus(std::ostream&, const std::string& margin, ReportInterface&);

    private:

        // Portable properties
        bool        _is_open;
        bool        _info_only;
        TunerType   _tuner_type;
        std::string _device_name;    // Used to open the tuner
        std::string _device_info;    // Device-specific, can be empty
        MilliSecond _signal_timeout;
        bool        _signal_timeout_silent;
        MilliSecond _receive_timeout;
        DeliverySystemSet _delivery_systems;

#if defined(__linux) || defined(DOXYGEN) // Linux properties

        std::string         _frontend_name;    // Frontend device name
        std::string         _demux_name;       // Demux device name
        std::string         _dvr_name;         // DVR device name
        int                 _frontend_fd;      // Frontend device file descriptor
        int                 _demux_fd;         // Demux device file descriptor
        int                 _dvr_fd;           // DVR device file descriptor
        unsigned long       _demux_bufsize;    // Demux device buffer size
        ::dvb_frontend_info _fe_info;          // Front-end characteristics
        bool                _force_s2api;      // Force usage of S2API
        MilliSecond         _signal_poll;
        int                 _rt_signal;        // Receive timeout signal number
        ::timer_t           _rt_timer;         // Receive timeout timer
        bool                _rt_timer_valid;   // Receive timeout timer was created

        // Get current tuning parameters for specific tuners, return system error code
        ErrorCode getCurrentTuningDVBS(TunerParametersDVBS&);
        ErrorCode getCurrentTuningDVBC(TunerParametersDVBC&);
        ErrorCode getCurrentTuningDVBT(TunerParametersDVBT&);
        ErrorCode getCurrentTuningATSC(TunerParametersATSC&);

        // Clear tuner if S2API, return true on success, false on error
        bool dtvClear(ReportInterface&);

        // Discard all pending frontend events
        void discardFrontendEvents(ReportInterface&);

        // Tune for specific tuners, return true on success, false on error
        bool tuneDVBS(const TunerParametersDVBS&, ReportInterface&);
        bool tuneDVBC(const TunerParametersDVBC&, ReportInterface&);
        bool tuneDVBT(const TunerParametersDVBT&, ReportInterface&);
        bool tuneATSC(const TunerParametersATSC&, ReportInterface&);

#if defined(__s2api) || defined(DOXYGEN)
        // Convert between TSDuck and Linux S2API delivery systems.
        DeliverySystem fromLinuxDeliverySystem(::fe_delivery_system);
        ::fe_delivery_system toLinuxDeliverySystem(DeliverySystem);
#endif
#endif // linux
       
#if defined(__windows) || defined(DOXYGEN) // Windows properties

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

        size_t                   _sink_queue_size;     // Media sample queue size
        ComPtr <::IGraphBuilder> _graph;               // The graph (subclass of IFilterGraph)
        ComPtr <::IMediaControl> _media_control;       // ... interface of graph
        ComPtr <SinkFilter>      _sink_filter;         // Sink filter to TSDuck
        ComPtr <::IBaseFilter>   _provider_filter;     // Network provider filter
        std::string              _provider_name;       // ... filter friendly name
        ComPtr <::IBDA_NetworkProvider> _net_provider; // ... interface of provider_filter
        ComPtr <::ITuner>        _tuner;               // ... interface of provider_filter
        ComPtr <::ITuningSpace>  _tuning_space;        // ... associated to provider_filter
        std::string              _tuning_space_fname;  // ... friendly name
        std::string              _tuning_space_uname;  // ... unique name
        ComPtr <::IBaseFilter>   _tuner_filter;        // Tuner filter
        std::vector <ComPtr <::IBDA_DigitalDemodulator>>  _demods;   // ... all its demod interfaces
        std::vector <ComPtr <::IBDA_DigitalDemodulator2>> _demods2;  // ... all its demod (2nd gen) interfaces
        std::vector <ComPtr <::IBDA_SignalStatistics>>    _sigstats; // ... all its signal stat interfaces
        std::vector <ComPtr <::IKsPropertySet>>           _tunprops; // ... all its property set interfaces

        // Try to build the graph.
        // Return true on success, false on error
        bool buildGraph(::IMoniker* provider_moniker, ::IMoniker* tuner_moniker, ReportInterface&);

        // Try to build the part of the graph starting at the tee filter.
        // The specified base filter is either the tuner filter or some
        // other intermediate receiver filter downstream the tuner.
        // Return true on success, false on error.
        bool buildCaptureGraph(const ComPtr <::IBaseFilter>&, ReportInterface&);

        // Internal tune method, works also if the tuner is not in open state.
        // Return true on success, false on errors
        bool internalTune(const TunerParameters&, ReportInterface&);

        // Create an IDigitalLocator object for various types of DVB parameters.
        // Return true on success, false on errors
        bool createLocatorDVBS(ComPtr<::IDigitalLocator>&, const TunerParametersDVBS&, ReportInterface&);
        bool createLocatorDVBT(ComPtr<::IDigitalLocator>&, const TunerParametersDVBT&, ReportInterface&);
        bool createLocatorDVBC(ComPtr<::IDigitalLocator>&, const TunerParametersDVBC&, ReportInterface&);

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
    };
}

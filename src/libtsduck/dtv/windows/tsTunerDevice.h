//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Digital TV tuner physical device.
//!  One version of this class exists for each operating system.
//   ==> Windows implementation.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerBase.h"
#include "tsModulationArgs.h"
#include "tsAbortInterface.h"
#include "tsSafePtr.h"
#include "tsReport.h"
#include "tsTunerGraph.h"

namespace ts {
    //!
    //! Digital TV tuner physical device.
    //! One version of this class exists for each operating system.
    //! @ingroup hardware
    //!
    class TSDUCKDLL TunerDevice: public TunerBase
    {
        TS_NOBUILD_NOCOPY(TunerDevice);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        TunerDevice(DuckContext& duck);

        //!
        //! Destructor.
        //!
        virtual ~TunerDevice() override;

        // Implementation of TunerBase.
        virtual bool open(const UString& device_name, bool info_only) override;
        virtual bool close(bool silent = false) override;
        virtual bool isOpen() const override;
        virtual bool infoOnly() const override;
        virtual const DeliverySystemSet& deliverySystems() const override;
        virtual UString deviceName() const override;
        virtual UString deviceInfo() const override;
        virtual UString devicePath() const override;
        virtual bool getSignalState(SignalState& state) override;
        virtual bool tune(ModulationArgs& params) override;
        virtual bool start() override;
        virtual bool stop(bool silent = false) override;
        virtual void abort(bool silent = false) override;
        virtual size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort = nullptr) override;
        virtual bool getCurrentTuning(ModulationArgs& params, bool reset_unknown) override;
        virtual void setSignalTimeout(std::chrono::milliseconds t) override;
        virtual void setSignalTimeoutSilent(bool silent) override;
        virtual bool setReceiveTimeout(std::chrono::milliseconds t) override;
        virtual std::chrono::milliseconds receiveTimeout() const override;
        virtual void setSinkQueueSize(size_t s) override;
        virtual void setReceiverFilterName(const UString& name) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin = UString(), bool extended = false) override;

    private:
        bool                      _is_open = false;
        bool                      _info_only = false;
        UString                   _device_name {};    // Used to open the tuner
        UString                   _device_info {};    // Device-specific, can be empty
        UString                   _device_path {};    // System-specific device path.
        std::chrono::milliseconds _signal_timeout = DEFAULT_SIGNAL_TIMEOUT;
        bool                      _signal_timeout_silent = false;
        std::chrono::milliseconds _receive_timeout {};
        DeliverySystemSet         _delivery_systems {};
        volatile bool             _aborted = false;   // Reception was aborted
        size_t                    _sink_queue_size = DEFAULT_SINK_QUEUE_SIZE; // Media sample queue size
        TunerGraph                _graph {};          // The filter graph

        // Find one or more tuners. Exactly one of Tuner* or TunerPtrVector* must be non-zero.
        // If Tuner* is non-zero, find the first tuner (matching _device_name if not empty).
        // If _device_name is ":integer", use integer as device index in list of DVB devices.
        // If TunerPtrVector* is non- zero, find all tuners in the system.
        // Return true on success, false on error.
        static bool FindTuners(DuckContext& duck, TunerDevice*, TunerPtrVector*);

        // And let TunerBase call our FindTuners.
        friend class TunerBase;
    };
}

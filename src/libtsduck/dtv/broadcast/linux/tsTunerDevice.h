//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Digital TV tuner physical device.
//!  One version of this class exists for each operating system.
//   ==> Linux implementation.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerBase.h"
#include "tsSignalState.h"
#include "tsModulationArgs.h"
#include "tsAbortInterface.h"
#include "tsReport.h"
#include "tsDTVProperties.h"

namespace ts {
    //!
    //! Digital TV tuner physical device.
    //! One version of this class exists for each operating system.
    //! @ingroup libtsduck hardware
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
        virtual void setSignalTimeout(cn::milliseconds t) override;
        virtual void setSignalTimeoutSilent(bool silent) override;
        virtual bool setReceiveTimeout(cn::milliseconds t) override;
        virtual cn::milliseconds receiveTimeout() const override;
        virtual void setSignalPoll(cn::milliseconds t) override;
        virtual void setDemuxBufferSize(size_t s) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin = UString(), bool extended = false) override;

    private:
        bool                _is_open = false;
        bool                _info_only = false;
        UString             _device_name {};          // Used to open the tuner
        UString             _device_info {};          // Device-specific, can be empty
        UString             _device_path {};          // System-specific device path.
        cn::milliseconds    _signal_timeout = DEFAULT_SIGNAL_TIMEOUT;
        bool                _signal_timeout_silent = false;
        cn::milliseconds    _receive_timeout {};
        DeliverySystemSet   _delivery_systems {};
        volatile bool       _reading_dvr = false;     // Read operation in progree on dvr.
        volatile bool       _aborted = false;         // Tuner operation was aborted
        UString             _frontend_name {};        // Frontend device name
        UString             _demux_name {};           // Demux device name
        UString             _dvr_name {};             // DVR device name
        int                 _frontend_fd = -1;        // Frontend device file descriptor
        int                 _demux_fd = -1;           // Demux device file descriptor
        int                 _dvr_fd = -1;             // DVR device file descriptor
        unsigned long       _demux_bufsize = DEFAULT_DEMUX_BUFFER_SIZE;
        ::dvb_frontend_info _fe_info {};              // Front-end characteristics
        cn::milliseconds    _signal_poll = DEFAULT_SIGNAL_POLL;
        int                 _rt_signal = -1;          // Receive timeout signal number
        ::timer_t           _rt_timer = nullptr;      // Receive timeout timer
        bool                _rt_timer_valid = false;  // Receive timeout timer was created
        bool                _voltage_on = false;      // Satellite tuner voltage was turned on

        // Clear tuner, return true on success, false on error
        bool dtvClear();

        // Perform a tune operation.
        bool dtvTune(DTVProperties&);

        // Discard all pending frontend events
        void discardFrontendEvents();

        // Get frontend status, encapsulate weird error management.
        bool getFrontendStatus(::fe_status_t&);

        // Hard close of the tuner, report can be null.
        void hardClose(Report*);

        // Setup the dish for satellite tuners.
        bool dishControl(const ModulationArgs&, const LNB::Transposition&);

        // Setup the Unicable multiswitch for satellite
        bool configUnicableSwitch(const ModulationArgs&);

        // Extract DTV_STAT_* properties and store it into a SignalState.
        static void GetStat(SignalState& state, std::optional<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd);
        static void GetStatRatio(SignalState& state, std::optional<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd1, uint32_t cmd2);
    };
}

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
#include "tsSafePtr.h"
#include "tsReport.h"
#include "tsDTVProperties.h"

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
        virtual void setSignalTimeout(MilliSecond t) override;
        virtual void setSignalTimeoutSilent(bool silent) override;
        virtual bool setReceiveTimeout(MilliSecond t) override;
        virtual MilliSecond receiveTimeout() const override;
        virtual void setSignalPoll(MilliSecond t) override;
        virtual void setDemuxBufferSize(size_t s) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin = UString(), bool extended = false) override;

    private:
        bool                _is_open;
        bool                _info_only;
        UString             _device_name;      // Used to open the tuner
        UString             _device_info;      // Device-specific, can be empty
        UString             _device_path;      // System-specific device path.
        MilliSecond         _signal_timeout;
        bool                _signal_timeout_silent;
        MilliSecond         _receive_timeout;
        DeliverySystemSet   _delivery_systems;
        volatile bool       _reading_dvr;      // Read operation in progree on dvr.
        volatile bool       _aborted;          // Tuner operation was aborted
        UString             _frontend_name;    // Frontend device name
        UString             _demux_name;       // Demux device name
        UString             _dvr_name;         // DVR device name
        int                 _frontend_fd;      // Frontend device file descriptor
        int                 _demux_fd;         // Demux device file descriptor
        int                 _dvr_fd;           // DVR device file descriptor
        unsigned long       _demux_bufsize;    // Demux device buffer size
        ::dvb_frontend_info _fe_info;          // Front-end characteristics
        MilliSecond         _signal_poll;
        int                 _rt_signal;        // Receive timeout signal number
        ::timer_t           _rt_timer;         // Receive timeout timer
        bool                _rt_timer_valid;   // Receive timeout timer was created

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

        // Extract DTV_STAT_* properties and store it into a SignalState.
        static void GetStat(SignalState& state, Variable<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd);
        static void GetStatRatio(SignalState& state, Variable<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd1, uint32_t cmd2);
    };
}

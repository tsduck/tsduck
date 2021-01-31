//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Digital TV tuner.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerBase.h"
#include "tsModulationArgs.h"
#include "tsAbortInterface.h"
#include "tsSafePtr.h"
#include "tsReport.h"

namespace ts {

    //!
    //! General-purpose implementation of a digital TV tuner.
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
    class TSDUCKDLL Tuner: public TunerBase
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

        // Inherited methods.
        virtual bool open(const UString& device_name, bool info_only, Report& report) override;
        virtual bool close(Report& report) override;
        virtual bool isOpen() const { return _is_open; }
        virtual bool infoOnly() const { return _info_only; }
        virtual const DeliverySystemSet& deliverySystems() const { return _delivery_systems; }
        virtual UString deviceName() const { return _device_name; }
        virtual UString deviceInfo() const { return _device_info; }
        virtual UString devicePath() const { return _device_path; }
        virtual bool signalLocked(Report& report) override;
        virtual int signalStrength(Report& report) override;
        virtual int signalQuality(Report& report) override;
        virtual bool tune(ModulationArgs& params, Report& report) override;
        virtual bool start(Report& report) override;
        virtual bool stop(Report& report) override;
        virtual void abort() override;
        virtual size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report) override;
        virtual bool getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report) override;
        virtual void setSignalTimeout(MilliSecond t) override;
        virtual void setSignalTimeoutSilent(bool silent) override;
        virtual bool setReceiveTimeout(MilliSecond t, Report& report) override;
        virtual MilliSecond receiveTimeout() const { return _receive_timeout; }
        virtual void setSignalPoll(MilliSecond t) override;
        virtual void setDemuxBufferSize(size_t s) override;
        virtual void setSinkQueueSize(size_t s) override;
        virtual void setReceiverFilterName(const UString& name) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin, Report& report, bool extended = false) override;

    private:
        TunerBase* _device;    // Physical tuner device.
        TunerBase* _emulator;  // File-based tuner emulator.

        // Allocate a physical tuner (depend on implementations).
        TunerBase* allocateDevice();

        // Private members.
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

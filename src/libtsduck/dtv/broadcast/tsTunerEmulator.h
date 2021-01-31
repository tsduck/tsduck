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
//!  Digital TV tuner emulator.
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
    //! Digital TV tuner emulator.
    //! @ingroup hardware
    //!
    //! @@ TO BE COMPLETED @@
    //!
    class TSDUCKDLL TunerEmulator: public TunerBase
    {
        TS_NOBUILD_NOCOPY(TunerEmulator);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        TunerEmulator(DuckContext& duck);

        //!
        //! Destructor.
        //!
        ~TunerEmulator();

        // Implementation of TunerBase.
        // Some methods are not implemented, left to the default method in base class.
        virtual bool open(const UString& device_name, bool info_only, Report& report) override;
        virtual bool close(Report& report) override;
        virtual bool isOpen() const override;
        virtual bool infoOnly() const override;
        virtual const DeliverySystemSet& deliverySystems() const override;
        virtual UString deviceName() const override;
        virtual UString deviceInfo() const override;
        virtual UString devicePath() const override;
        virtual bool signalLocked(Report& report) override;
        virtual int signalStrength(Report& report) override;
        virtual int signalQuality(Report& report) override;
        virtual bool tune(ModulationArgs& params, Report& report) override;
        virtual bool start(Report& report) override;
        virtual bool stop(Report& report) override;
        virtual size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report) override;
        virtual bool getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin, Report& report, bool extended = false) override;

    private:
        DeliverySystemSet _delivery_systems;  // Collection of all delivery systems.
        UString           _file_path;         // Main XML file path.
        bool              _info_only;         // Open mode, useless here, just informational.
    };
}

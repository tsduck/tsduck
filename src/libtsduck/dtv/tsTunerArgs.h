//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Parameters for tuners and their command-line definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"

namespace ts {

    class Tuner;

    //!
    //! Parameters for tuners and their command-line definitions.
    //! @ingroup hardware
    //!
    //! All values may be "set" or "unset", depending on command line arguments.
    //! All options for all types of tuners are included here.
    //!
    class TSDUCKDLL TunerArgs : public ModulationArgs
    {
    public:
        // Public fields
        UString     device_name;        //!< Name of tuner device.
        MilliSecond signal_timeout;     //!< Signal locking timeout in milliseconds.
        MilliSecond receive_timeout;    //!< Packet received timeout in milliseconds.
#if defined(TS_LINUX) || defined(DOXYGEN)
        size_t      demux_buffer_size;  //!< Demux buffer size in bytes (Linux-specific).
#endif
#if defined(TS_WINDOWS) || defined(DOXYGEN)
        size_t      demux_queue_size;   //!< Max number of queued media samples (Windows-specific).
        UString     receiver_name;      //!< Name of the DirectShow receiver to use (Windows-specific).
#endif

        //!
        //! Default constructor.
        //! @param [in] info_only If true, the tuner will not be used to tune, just to get information.
        //! @param [in] allow_short_options If true, allow short one-letter options.
        //!
        TunerArgs(bool info_only = false, bool allow_short_options = true);

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        // Overridden from superclass.
        virtual void reset() override;

        //!
        //! Open a tuner and configure it according to the parameters in this object.
        //! @param [in,out] tuner The tuner to open and configure.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool configureTuner(Tuner& tuner, Report& report) const;

    private:
        bool _info_only;
    };
}

//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Command line arguments for DVB tuners
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsVariable.h"
#include "tsModulation.h"
#include "tsLNB.h"

namespace ts {

    class TSDUCKDLL TunerArgs
    {
    public:
        // All values may be "set" or "unset", depending on command line arguments.
        // All options for all types of tuners are included here.

        // Public fields
        Variable<std::string>       zap_specification;  // Linux DVB zap format
        Variable<std::string>       channel_name;       // Use transponder containing this channel
        Variable<std::string>       zap_file_name;      // Where channel_name is located
        Variable<uint64_t>            frequency;          // Always in Hz
        Variable<Polarization>      polarity;
        Variable<LNB>               lnb;
        Variable<SpectralInversion> inversion;
        Variable<uint32_t>            symbol_rate;
        Variable<InnerFEC>          inner_fec;
        Variable<size_t>            satellite_number;
        Variable<Modulation>        modulation;
        Variable<BandWidth>         bandwidth;
        Variable<InnerFEC>          fec_hp;
        Variable<InnerFEC>          fec_lp;
        Variable<TransmissionMode>  transmission_mode;
        Variable<GuardInterval>     guard_interval;
        Variable<Hierarchy>         hierarchy;
        Variable<DeliverySystem>    delivery_system;
        Variable<Pilot>             pilots;
        Variable<RollOff>           roll_off;

        // Check if actual tuning information is set
        bool hasTuningInfo() const {return frequency.set() || zap_specification.set() || channel_name.set();}

        // Reset all values, they become "unset"
        void reset();

        // Load arguments from command line.
        // Args error indicator is set in case of incorrect arguments
        void load (Args& args);

        // Define command line options in an Args.
        static void DefineOptions (Args& args, bool allow_short_options = true);

        // Add help about command line options in an Args
        static void AddHelp (Args& args, bool allow_short_options = true);

        // Default zap file name for a given tuner type.
        // Return an empty string if there is no default.
        static std::string DefaultZapFile (TunerType);
    };
}

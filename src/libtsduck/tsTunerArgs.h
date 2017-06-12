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
    //!
    //! Command line arguments for DVB tuners.
    //!
    //! All values may be "set" or "unset", depending on command line arguments.
    //! All options for all types of tuners are included here.
    //!
    class TSDUCKDLL TunerArgs
    {
    public:
        // Public fields
        Variable<std::string>       zap_specification;  //!< Linux DVB zap format.
        Variable<std::string>       channel_name;       //!< Use transponder containing this channel.
        Variable<std::string>       zap_file_name;      //!< Where channel_name is located.
        Variable<uint64_t>          frequency;          //!< Frequency in Hz.
        Variable<Polarization>      polarity;           //!< Polarity.
        Variable<LNB>               lnb;                //!< Local dish LNB for frequency adjustment.
        Variable<SpectralInversion> inversion;          //!< Spectral inversion.
        Variable<uint32_t>          symbol_rate;        //!< Symbol rate.
        Variable<InnerFEC>          inner_fec;          //!< Error correction.
        Variable<size_t>            satellite_number;   //!< For DiSeqC (usually 0).
        Variable<Modulation>        modulation;         //!< Constellation or modulation type.
        Variable<BandWidth>         bandwidth;          //!< Bandwidth.
        Variable<InnerFEC>          fec_hp;             //!< High priority stream code rate.
        Variable<InnerFEC>          fec_lp;             //!< Low priority stream code rate.
        Variable<TransmissionMode>  transmission_mode;  //!< Transmission mode.
        Variable<GuardInterval>     guard_interval;     //!< Guard interval.
        Variable<Hierarchy>         hierarchy;          //!< Hierarchy.
        Variable<DeliverySystem>    delivery_system;    //!< Delivery system (DS_DVB_*).
        Variable<Pilot>             pilots;             //!< Presence of pilots (DVB-S2 only).
        Variable<RollOff>           roll_off;           //!< Roll-off factor (DVB-S2 only).

        //!
        //! Default constructor.
        //!
        TunerArgs();

        //!
        //! Check if actual tuning information is set.
        //! @return True if actual tuning information is set.
        //!
        bool hasTuningInfo() const
        {
            return frequency.set() || zap_specification.set() || channel_name.set();
        }

        //!
        //! Reset all values, they become "unset"
        //!
        void reset();

        //!
        //! Define command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] allow_short_options If true, allow short one-letter options.
        //! If false, disable them, possibly to avoid interferences with other options.
        //!
        static void DefineOptions(Args& args, bool allow_short_options = true);

        //!
        //! Add help about command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] allow_short_options If true, allow short one-letter options.
        //! If false, disable them, possibly to avoid interferences with other options.
        //!
        static void AddHelp(Args& args, bool allow_short_options = true);

        //!
        //! Load arguments from command line.
        //! @param [in,out] args Command line arguments.
        //! Args error indicator is set in case of incorrect arguments.
        //!
        void load(Args& args);

        //!
        //! Default zap file name for a given tuner type.
        //! @param [in] type Tuner type.
        //! @return The default zap file or an empty string if there is no default.
        //!
        static std::string DefaultZapFile(TunerType type);
    };
}

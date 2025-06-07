//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSC.h"
#include "tsModulationArgs.h"


//----------------------------------------------------------------------------
// Compute a bitrate from a ModulationArgs for ATSC.
//----------------------------------------------------------------------------

TS_REGISTER_BITRATE_CALCULATOR(ts::GetBitRateATSC, {ts::DS_ATSC});

bool ts::GetBitRateATSC(BitRate& bitrate, const ModulationArgs& args)
{
    if (args.delivery_system == DS_ATSC) {
        // Only two modulation values are available for ATSC.
        const Modulation mod = args.modulation.value_or(ModulationArgs::DEFAULT_MODULATION_ATSC);
        if (mod == VSB_8) {
            bitrate = 19'392'658;
            return true;
        }
        else if (mod == VSB_16) {
            bitrate = 38'785'317;
            return true;
        }
    }
    return false;
}

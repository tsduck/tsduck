//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSP.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSP::TSP(int max_severity) :
    Report(max_severity),
    _use_realtime(false),
    _tsp_bitrate(0),
    _tsp_bitrate_confidence(BitRateConfidence::LOW),
    _tsp_timeout(Infinite),
    _tsp_aborting(false),
    _total_packets(0),
    _plugin_packets(0)
{
}

ts::TSP::~TSP()
{
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

bool ts::TSP::aborting() const
{
    return _tsp_aborting;
}
